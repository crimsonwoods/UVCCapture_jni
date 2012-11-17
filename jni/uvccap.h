#ifndef UVC_CAPTURE_H
#define UVC_CAPTURE_H

#include<stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum uvcc_error_t {
    NOERROR = 0,
    INVALID_ARGUMENTS = 100,
    INVALID_FORMAT_ARGUMENTS,
	INVALID_STATUS,
    VIDEO_DEVICE_BUSY,
    VIDEO_DEVICE_OPEN_FAILED,
    VIDEO_DEVICE_NOCAPS,
    VIDEO_DEVICE_NOCROPCAPS,
    VIDEO_DEVICE_CAPTURE_NOT_SUPPORTED,
    VIDEO_DEVICE_CROPPING_FAILED,
    VIDEO_DEVICE_ENUM_FORMAT_FAILED,
    VIDEO_DEVICE_QUERY_BUFFER_FAILED,
    VIDEO_DEVICE_STREAMING_FAILED,
    IO_METHOD_NOT_SUPPORTED,
    IO_ERROR,
    IO_FILE_NOT_CREATED,
    MEMORY_MAPPING_FAILED,
    MEMORY_QUEUEING_FAILED,
    MEMORY_DEQUEUEING_FAILED,
    INSUFFICIENT_MEMORY,
    NOT_PERMITTED,
	NO_MORE_DATA,
	PREVIEW_SIZE_NOT_SUPPORTED,
} uvcc_error_t;

typedef enum uvcc_pixel_format_t {
	UVCC_PIX_FMT_RGB565 = 0,
	UVCC_PIX_FMT_RGB32,
	UVCC_PIX_FMT_BGR32,
	UVCC_PIX_FMT_YUYV,
	UVCC_PIX_FMT_UYVY,
	UVCC_PIX_FMT_YUV420,
	UVCC_PIX_FMT_YUV410,
	UVCC_PIX_FMT_YUV422P,
	UVCC_PIX_FMT_COUNT, // count of pixel formats.
} uvcc_pixel_format_t;

typedef struct uvcc_preview_size_t {
	uint32_t width;
	uint32_t height;
} uvcc_preview_size_t;

typedef void const* uvcc_handle_t;

extern int  uvcc_open_video_device(uvcc_handle_t *handle, char const * const path);
extern void uvcc_close_video_device(uvcc_handle_t handle);
extern int  uvcc_init_video_device(uvcc_handle_t handle, uint32_t width, uint32_t height, uvcc_pixel_format_t pixel_format);
extern int  uvcc_start_capture(uvcc_handle_t dev);
extern void uvcc_stop_capture(uvcc_handle_t dev);
extern int  uvcc_capture(uvcc_handle_t handle, uint8_t * const buf, size_t buf_size);
extern uint32_t uvcc_get_frame_size(uvcc_handle_t handle);
extern uint32_t uvcc_get_frame_width(uvcc_handle_t handle);
extern uint32_t uvcc_get_frame_height(uvcc_handle_t handle);
extern uint32_t uvcc_get_pixel_format(uvcc_handle_t handle);
extern int  uvcc_enum_preview_size(uvcc_handle_t handle, int index, uvcc_pixel_format_t pixel_format, uvcc_preview_size_t *size);

#ifdef __cplusplus
}
#endif

#endif
