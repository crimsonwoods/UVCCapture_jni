#include "uvccap_jni.h"
#include "uvccap.h"
#include <stdint.h>
#include <android/log.h>

#define LOG_TAG "libuvccap"

#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__);

static void throw_RuntimeException(JNIEnv *env, char const * const message);
static void throw_IOException(JNIEnv *env, char const * const message); 

#define TO_HANDLE(h) ((uvcc_handle_t)(intptr_t)h)

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_getPixelFormat
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1getPixelFormat
  (JNIEnv *env, jobject thiz, jlong handle)
{
	return uvcc_get_pixel_format(TO_HANDLE(handle));
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_getFrameSize
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1getFrameSize
  (JNIEnv *env, jobject thiz, jlong handle)
{
	return uvcc_get_frame_size(TO_HANDLE(handle));
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_getWidth
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1getWidth
  (JNIEnv *env, jobject thiz, jlong handle)
{
	return uvcc_get_frame_width(TO_HANDLE(handle));
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_getHeight
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1getHeight
  (JNIEnv *env, jobject thiz, jlong handle)
{
	return uvcc_get_frame_height(TO_HANDLE(handle));
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_open
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1open
  (JNIEnv *env, jobject thiz, jstring path)
{
	const char *nativePath;
	int result = NOERROR;
	uvcc_handle_t handle = NULL;

	nativePath = (*env)->GetStringUTFChars(env, path, 0);

	result = uvcc_open_video_device(&handle, nativePath);

	(*env)->ReleaseStringUTFChars(env, path, nativePath);

	if (NOERROR != result) {
		throw_IOException(env, "Video device can't open.");
	}

	return (jlong)(intptr_t)handle;
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_init
 * Signature: (JIII)V
 */
JNIEXPORT void JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1init
  (JNIEnv *env, jobject thiz, jlong handle, jint width, jint height, jint pixfmt)
{
	int result = NOERROR;
	LOGD("init(width=%d, height=%d, pixfmt=%d)", width, height, pixfmt);
	result = uvcc_init_video_device(TO_HANDLE(handle), width, height, pixfmt);
	if (NOERROR != result) {
		throw_RuntimeException(env, "Video device can't init.");
	}
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1close
  (JNIEnv *env, jobject thiz, jlong handle)
{
	uvcc_close_video_device(TO_HANDLE(handle));
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_capture
 * Signature: (J[B)V
 */
JNIEXPORT void JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1capture
  (JNIEnv *env, jobject thiz, jlong handle, jbyteArray buf)
{
	jboolean isCopy = JNI_FALSE;
	void *ptr = NULL;
	jsize size = 0;
	int result = NOERROR;

	if (NULL == buf) {
		return;
	}

	size = (*env)->GetArrayLength(env, buf);
	ptr = (*env)->GetPrimitiveArrayCritical(env, buf, &isCopy);

	result = uvcc_capture(TO_HANDLE(handle), ptr, size);
	if ((NOERROR == result) && (JNI_FALSE != isCopy)) {
		(*env)->ReleasePrimitiveArrayCritical(env, buf, ptr, JNI_COMMIT);
	} else {
		(*env)->ReleasePrimitiveArrayCritical(env, buf, ptr, 0);
	}
	if (NOERROR != result) {
		throw_RuntimeException(env, "Capture failed.");
	}
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_start
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1start
  (JNIEnv *env, jobject thiz, jlong handle)
{
	int result = uvcc_start_capture(TO_HANDLE(handle));
	if (NOERROR != result) {
		throw_RuntimeException(env, "Could not start capturing.");
	}
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_stop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1stop
  (JNIEnv *env, jobject thiz, jlong handle)
{
	uvcc_stop_capture(TO_HANDLE(handle));
}

/*
 * Class:     net_crimsonwoods_android_libs_uvccap_UVCCamera
 * Method:    n_enumFrameSize
 * Signature: (JII)Lcrimsonwoods/android/libs/uvccap/UVCCamera/FrameSize;
 */
JNIEXPORT jobject JNICALL Java_net_crimsonwoods_android_libs_uvccap_UVCCamera_n_1enumFrameSize
  (JNIEnv *env, jobject thiz, jlong handle, jint index, jint pixfmt)
{
	uvcc_preview_size_t size;
	int const err = uvcc_enum_preview_size(TO_HANDLE(handle), index, pixfmt, &size);
	if (err != NOERROR) {
		return NULL;
	}
	jclass cls = (*env)->FindClass(env, "net/crimsonwoods/android/libs/uvccap/UVCCamera$FrameSize");
	if (NULL == cls) {
		// ClassNotFoundException will throw by JVM.
		return NULL;
	}
	jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "()V");
	if (NULL == ctor) {
		(*env)->DeleteLocalRef(env, cls);
		return NULL;
	}
	jfieldID field_w = (*env)->GetFieldID(env, cls, "width", "I");
	jfieldID field_h = (*env)->GetFieldID(env, cls, "height", "I");
	if (!field_w || !field_h) {
		(*env)->DeleteLocalRef(env, cls);
		return NULL;
	}
	jobject ret = (*env)->NewObject(env, cls, ctor);
	if (NULL != ret) {
		(*env)->SetIntField(env, ret, field_w, size.width);
		(*env)->SetIntField(env, ret, field_h, size.height);
	}
	return ret;
}

static void throw_exception(JNIEnv *env, char const * const cls, char const * const message) {
	jclass ioe_cls = (*env)->FindClass(env, cls);
	if (NULL == ioe_cls) {
		return;
	}
	(*env)->ThrowNew(env, ioe_cls, message);
	(*env)->DeleteLocalRef(env, ioe_cls);
}

static void throw_IOException(JNIEnv *env, char const * const message) {
	throw_exception(env, "java/io/IOException", message);
}

static void throw_RuntimeException(JNIEnv *env, char const * const message) {
	throw_exception(env, "java/lang/RuntimeException", message);
}

