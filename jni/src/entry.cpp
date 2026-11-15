#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cmath>
#include <cstring>

#define LOG_TAG "ZeninESP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- ОФФСЕТЫ ИЗ ДАМПА (1.1.0) ---
namespace off {
    // Статические поля (RVA в libil2cpp.so)
    // Если не сработает, будем искать через il2cpp API
    constexpr uintptr_t player_manager_rva = 0xCD2F9B0; // предположительно ClientWorld/PlayerManager
    constexpr uintptr_t main_camera_rva = 0xCD2F9B8;    // примерный адрес, нужно уточнить

    // Оффсеты полей внутри классов
    constexpr uintptr_t lazy_instance = 0x0;
    constexpr uintptr_t local_player = 0x40;
    constexpr uintptr_t player_list_dict = 0x28; // или 0x30, где список всех
    constexpr uintptr_t player_biped = 0x30;
    constexpr uintptr_t biped_head = 0x20;
    constexpr uintptr_t transform_pos = 0x38; // Vector3

    // Камера
    constexpr uintptr_t camera_component = 0x20;
    constexpr uintptr_t cam_view_matrix = 0x70; // native offset
}

// Простые векторы
struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

// Глобальные указатели
static EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
static uintptr_t base_addr = 0;
static int screen_w = 0, screen_h = 0;
static GLuint shader_program = 0;

// Простой шейдер для рисования линий
void DrawRect(float x, float y, float w, float h, float r, float g, float b) {
    GLfloat verts[] = { x, y, x+w, y, x+w, y+h, x, y+h };
    glUseProgram(shader_program);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableVertexAttribArray(0);
}

bool WorldToScreen(Vector3 pos, Vector2& screen, float* matrix) {
    float w = matrix[3]*pos.x + matrix[7]*pos.y + matrix[11]*pos.z + matrix[15];
    if (w < 0.1f) return false;
    float x = matrix[0]*pos.x + matrix[4]*pos.y + matrix[8]*pos.z + matrix[12];
    float y = matrix[1]*pos.x + matrix[5]*pos.y + matrix[9]*pos.z + matrix[13];
    screen.x = (screen_w / 2.0f) * (1.0f + x / w);
    screen.y = (screen_h / 2.0f) * (1.0f - y / w);
    return true;
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (screen_w == 0) {
        eglQuerySurface(dpy, surf, EGL_WIDTH, &screen_w);
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &screen_h);
        LOGI("Screen: %dx%d", screen_w, screen_h);
    }
    if (!base_addr) {
        base_addr = (uintptr_t)dlopen("libil2cpp.so", RTLD_NOW);
        LOGI("libil2cpp base: %p", (void*)base_addr);
    }

    if (base_addr) {
        glViewport(0, 0, screen_w, screen_h);

        // --- ПРОБУЕМ ПРОЧИТАТЬ КАМЕРУ И ИГРОКОВ ---
        // Если игра крашится на этих строчках, значит RVA неверные, и мы будем их искать.
        // Пока для теста нарисуем квадрат в центре.
        DrawRect(screen_w/2 - 50, screen_h/2 - 50, 100, 100, 1.0f, 0.0f, 0.0f);
    }

    return orig_eglSwapBuffers(dpy, surf);
}

GLuint CompileShader(const char* src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

void InitGL() {
    const char* vShader = "attribute vec2 pos; void main() { gl_Position = vec4(pos / vec2(1080.0, 2400.0) * 2.0 - 1.0, 0.0, 1.0); }";
    const char* fShader = "precision mediump float; void main() { gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); }";
    GLuint vs = CompileShader(vShader, GL_VERTEX_SHADER);
    GLuint fs = CompileShader(fShader, GL_FRAGMENT_SHADER);
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vs);
    glAttachShader(shader_program, fs);
    glLinkProgram(shader_program);
    glBindAttribLocation(shader_program, 0, "pos");
}

extern "C" void _() {
    void* handle = dlopen("libEGL.so", RTLD_LAZY);
    orig_eglSwapBuffers = (EGLBoolean(*)(EGLDisplay, EGLSurface))dlsym(handle, "eglSwapBuffers");
    InitGL();
    LOGI("ZeninESP Loaded! Overlay ready.");
    
    // ПРИМЕЧАНИЕ: Чтобы хук реально перехватывал eglSwapBuffers,
    // нужно использовать DobbyHook или inline-патчинг. 
    // В лоадере от Komarq (liblauncher) это уже может быть сделано,
    // но если мы пишем свой модуль с нуля, нужно добавить Dobby в сборку.
    // Пока для теста мы просто логируем загрузку.
}
