#include "colorconv.h"
#include <stdint.h>

static void throw_IllegalArgumentException(JNIEnv *env, char const * const message);
static void throw_NullPointerException(JNIEnv *env, char const * const message);

static uint32_t yuv2rgba(uint8_t y, uint8_t u, uint8_t v);

/*
 * Class:     net_ubilabo_android_apps_uvccapture_ColorConverter
 * Method:    yuyvtorgb
 * Signature: ([I[BII)V
 */
JNIEXPORT void JNICALL Java_net_ubilabo_android_apps_uvccapture_ColorConverter_yuyvtorgb
  (JNIEnv *env, jclass cls, jintArray rgba, jbyteArray yuyv, jint width, jint height)
{
	jbyte *yuyv_ptr = NULL;
	jint  *rgba_ptr = NULL;
	jboolean is_yuyv_copy = JNI_FALSE;
	jboolean is_rgba_copy = JNI_FALSE;
	int x, y;
	int const w = width / 2;
	int const h = height;
	uint32_t yuyv_ofst = 0;
	uint32_t rgba_ofst = 0;

	if (NULL == rgba) {
		throw_NullPointerException(env, "'rgba' have to be set not null.");
		return;
	}

	if (NULL == yuyv) {
		throw_NullPointerException(env, "'yuyv' have to be set not null.");
		return;
	}

	if (0 != (width & 0x01)) {
		throw_IllegalArgumentException(env, "'width' have to be even number.");
		return;
	}

	rgba_ptr = (*env)->GetPrimitiveArrayCritical(env, rgba, &is_rgba_copy);
	yuyv_ptr = (*env)->GetPrimitiveArrayCritical(env, yuyv, &is_yuyv_copy);

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			uint8_t y1 = yuyv_ptr[yuyv_ofst++];
			uint8_t u1 = yuyv_ptr[yuyv_ofst++];
			uint8_t y2 = yuyv_ptr[yuyv_ofst++];
			uint8_t v1 = yuyv_ptr[yuyv_ofst++];

			rgba_ptr[rgba_ofst++] = yuv2rgba(y1, u1, v1);
			rgba_ptr[rgba_ofst++] = yuv2rgba(y2, u1, v1);
		}
	}

	(*env)->ReleasePrimitiveArrayCritical(env, yuyv, yuyv_ptr, (JNI_FALSE != is_yuyv_copy) ? JNI_COMMIT : 0);
	(*env)->ReleasePrimitiveArrayCritical(env, rgba, rgba_ptr, (JNI_FALSE != is_rgba_copy) ? JNI_COMMIT : 0);
}

static void throw_exception(JNIEnv *env, char const * const cls, char const * const message)
{
	jclass ioe_cls = (*env)->FindClass(env, cls);
	if (NULL == ioe_cls) {
		return;
	}
	(*env)->ThrowNew(env, ioe_cls, message);
	(*env)->DeleteLocalRef(env, ioe_cls);
}

static void throw_IllegalArgumentException(JNIEnv *env, char const * const message)
{
	throw_exception(env, "java/lang/IllegalArgumentException", message);
}

static void throw_NullPointerException(JNIEnv *env, char const * const message)
{
	throw_exception(env, "java/lang/NullPointerException", message);
}

static int32_t clamp(int32_t value, int32_t min, int32_t max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

static inline uint32_t yuv2rgba(uint8_t y, uint8_t u, uint8_t v)
{
	int const iy = 1192 * clamp(y - 16, 0, 255);
	int const iu = u - 128;
	int const iv = v - 128;

	int32_t const r = clamp(iy + 1634 * iv +    0 * iu, 0, 262143);
	int32_t const g = clamp(iy -  833 * iv -  400 * iu, 0, 262143);
	int32_t const b = clamp(iy +    0 * iv + 2066 * iu, 0, 262143);

	return 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
}

