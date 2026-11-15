#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "ZeninESP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" __attribute__((visibility("default"))) void _() {
    // Просто пишем в лог, что мы загрузились
    LOGI("ZeninESP: LOADED SUCCESSFULLY");
    
    // Пытаемся найти libil2cpp
    void* base = dlopen("libil2cpp.so", RTLD_NOW);
    if (base) {
        LOGI("ZeninESP: libil2cpp found at %p", base);
    } else {
        LOGI("ZeninESP: libil2cpp NOT found");
    }
}
