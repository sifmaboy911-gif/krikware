LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := esp
LOCAL_SRC_FILES := src/entry.cpp
LOCAL_LDLIBS    := -llog -lEGL -lGLESv2 -landroid

# Экспортируем функцию _, чтобы твой инжектор её увидел
LOCAL_LDFLAGS   += -Wl,-E
LOCAL_CFLAGS    += -fvisibility=default

include $(BUILD_SHARED_LIBRARY)
