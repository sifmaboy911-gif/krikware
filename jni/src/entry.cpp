#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cmath>

#define LOG_TAG "ZeninESP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- Оффсеты ---
namespace off {
    // Смещение до ClientWorld в libil2cpp (из твоих дампов)
    constexpr uintptr_t client_world = 0xCD2F9B0;
    
    // Смещение до LocalPlayer в PlayerManager
    constexpr uintptr_t local_player = 0x1234; // ЗАМЕНИ на реальный оффсет
    
    // Смещение до списка игроков
    constexpr uintptr_t player_list = 0x1234;  // ЗАМЕНИ
    
    // Смещение до матрицы камеры (View Matrix)
    constexpr uintptr_t view_matrix = 0x1234;  // ЗАМЕНИ
}

// Простые векторы
struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

// Глобальные указатели
static EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
static uintptr_t libil2cpp_base = 0;
static int screen_w = 0;
static int screen_h = 0;

// Простой шейдер для рисования линий
GLuint shader_program = 0;

void DrawRect(float x, float y, float w, float h, float r, float g, float b) {
    // Вершины для GL_LINE_LOOP (контур)
    GLfloat verts[] = {
        x, y,         // Верхний левый
        x + w, y,     // Верхний правый
        x + w, y + h, // Нижний правый
        x, y + h      // Нижний левый
    };
    
    glUseProgram(shader_program);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableVertexAttribArray(0);
}

bool WorldToScreen(Vector3 world, Vector2& screen, float* matrix) {
    // Матрица Unity 4x4 (column-major)
    float w = matrix[3] * world.x + matrix[7] * world.y + matrix[11] * world.z + matrix[15];
    if (w < 0.1f) return false;
    
    float x = matrix[0] * world.x + matrix[4] * world.y + matrix[8] * world.z + matrix[12];
    float y = matrix[1] * world.x + matrix[5] * world.y + matrix[9] * world.z + matrix[13];
    
    screen.x = (screen_w / 2.0f) * (1.0f + x / w);
    screen.y = (screen_h / 2.0f) * (1.0f - y / w);
    return true;
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    // 1. Получаем разрешение экрана (один раз)
    if (screen_w == 0) {
        eglQuerySurface(dpy, surf, EGL_WIDTH, &screen_w);
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &screen_h);
    }
    
    // 2. Ищем базу libil2cpp
    if (!libil2cpp_base) {
        libil2cpp_base = (uintptr_t)dlopen("libil2cpp.so", RTLD_NOW);
    }

    // 3. Рисуем (если база найдена)
    if (libil2cpp_base) {
        // ВНИМАНИЕ: Тут должен быть код поиска игроков по твоим оффсетам.
        // Пока для теста просто нарисуем красный прямоугольник в центре экрана.
        glViewport(0, 0, screen_w, screen_h);
        DrawRect(screen_w/2 - 50, screen_h/2 - 50, 100, 100, 1.0f, 0.0f, 0.0f);
    }
    
    // 4. Вызываем оригинал, чтобы игра отрисовала кадр
    return orig_eglSwapBuffers(dpy, surf);
}

// Компиляция простого шейдера
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

// Точка входа для инжектора
extern "C" void _() {
    // Находим оригинальный eglSwapBuffers
    void* handle = dlopen("libEGL.so", RTLD_LAZY);
    orig_eglSwapBuffers = (EGLBoolean(*)(EGLDisplay, EGLSurface))dlsym(handle, "eglSwapBuffers");
    
    // Инициализируем шейдер
    InitGL();
    
    // Хукаем через Dobby (или просто патчим память, но Dobby надежнее)
    // ВНИМАНИЕ: В настоящем проекте тут надо использовать DobbyHook.
    // Для теста мы просто перезапишем указатель (это не сработает с EGL,
    // поэтому нужно будет добавить Dobby в сборку).
    
    // Пока для теста просто выведем лог, что мы загрузились.
    LOGI("ZeninESP loaded! Overlay is ready.");
}
