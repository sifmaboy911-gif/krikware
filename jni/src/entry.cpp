#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cmath>
#include <cstring>
#include <cstdint>

#define LOG_TAG "ZeninESP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- ОФФСЕТЫ ИЗ ДАМПА (1.1.0) ---
namespace off {
    constexpr uintptr_t player_manager_rva = 0xCD2F9B0; 
    constexpr uintptr_t main_camera_rva = 0xCD2F9B8;    
    constexpr uintptr_t lazy_instance = 0x0;
    constexpr uintptr_t local_player = 0x40;
    constexpr uintptr_t player_biped = 0x30;
    constexpr uintptr_t biped_head = 0x20;
    constexpr uintptr_t transform_pos = 0x38; 
    constexpr uintptr_t camera_component = 0x20;
    constexpr uintptr_t cam_view_matrix = 0x70; 
}

struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

static uintptr_t base_addr = 0;
static int screen_w = 0, screen_h = 0;
static GLuint shader_program = 0;

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

// --- ТОЧКА ВХОДА ---
extern "C" __attribute__((visibility("default"))) void _() {
    LOGI("ZeninESP: Entry point started.");
    
    // 1. Загружаем libil2cpp
    base_addr = (uintptr_t)dlopen("libil2cpp.so", RTLD_NOW);
    LOGI("ZeninESP: libil2cpp base: %p", (void*)base_addr);

    // 2. Инициализируем OpenGL (шaders)
    InitGL();
    LOGI("ZeninESP: OpenGL initialized.");
    
    // 3. Всё, больше ничего не делаем. Если это выполнится без краша,
    // инжектор вернет entry_result=0 и напишет MODULE_READY.
    // НО у нас в launch.sh нет ready, так что он просто завершится.
}
