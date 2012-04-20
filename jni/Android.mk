LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := uvccap
LOCAL_CFLAGS    := -Wall -Werror -O2
LOCAL_SRC_FILES := uvccap.c uvccap_jni.c
LOCAL_LDLIBS    += -llog

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE    := cconv
LOCAL_CFLAGS    := -Wall -Werror -O2
LOCAL_SRC_FILES := colorconv.c
LOCAL_LDLIBS    += -llog

include $(BUILD_SHARED_LIBRARY)
