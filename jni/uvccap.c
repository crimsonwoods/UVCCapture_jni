#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev.h>

#include <android/log.h>

#define LOG_TAG "uvccap"
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, fmt, ##__VA_ARGS__)

#include "uvccap.h"

/* Data structure and constant values */
#define DEF_PIXEL_FORMAT UVCC_PIX_FMT_YUYV

static uint32_t const PIXEL_FORMATS[UVCC_PIX_FMT_COUNT + 1] = {
	V4L2_PIX_FMT_RGB565,
	V4L2_PIX_FMT_RGB32,
	V4L2_PIX_FMT_BGR32,
	V4L2_PIX_FMT_YUYV,
	V4L2_PIX_FMT_UYVY,
	V4L2_PIX_FMT_YUV420,
	V4L2_PIX_FMT_YUV410,
	V4L2_PIX_FMT_YUV422P,
	0, // sentinel
};

typedef union pixel_format_name_t_ {
	uint32_t u;
	char name[4];
} pixel_format_name_t;

typedef struct video_buf_t_ {
	void    *addr;
	uint32_t size;
} video_buf_t;

typedef struct video_dev_t_ {
	int                    fd;
	struct v4l2_capability caps;
	struct v4l2_cropcap    cropcaps;
	struct v4l2_crop       crop;
	struct v4l2_format     format;
	video_buf_t           *buffers;
	int                    buffer_count;
	int                    is_capture_started;
} video_dev_t;

/* Internal APIs */
static void print_capability(struct v4l2_capability const *caps);
static void print_format_desc(struct v4l2_fmtdesc const *desc);
static uint32_t to_v4l2_pixel_format(int format);
static uint32_t from_v4l2_pixel_format(uint32_t format);
static int init_buffer(video_dev_t *dev);
static int read_frame(uint8_t * const buf, uint32_t buf_size, video_dev_t const *dev);

static int read_frame(uint8_t * const buf, uint32_t buf_size, video_dev_t const *dev) {
	struct v4l2_buffer v4l2_buf;
	int result = NOERROR;
	uint32_t size;

	assert(NULL != buf);
	assert(NULL != dev);

	memset(&v4l2_buf, 0, sizeof(v4l2_buf));
	v4l2_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;

	if (0 > ioctl(dev->fd, VIDIOC_DQBUF, &v4l2_buf)) {
		LOGE("Failed to dequeueing buffer (%s).", strerror(errno));
		return MEMORY_DEQUEUEING_FAILED;
	}

	assert(v4l2_buf.index < dev->buffer_count);

	size = buf_size < dev->buffers[v4l2_buf.index].size ? buf_size : dev->buffers[v4l2_buf.index].size;
	memcpy(buf, dev->buffers[v4l2_buf.index].addr, size);

	if (0 > ioctl(dev->fd, VIDIOC_QBUF, &v4l2_buf)) {
		LOGE("Failed to queueing buffer (%s).", strerror(errno));
		result = MEMORY_QUEUEING_FAILED;
	}

	return result;
}

static void print_capability(struct v4l2_capability const *caps) {
	assert(NULL != caps);

	char flags[512];

	memset(flags, 0, sizeof(flags));

	if (0 != (V4L2_CAP_VIDEO_CAPTURE & caps->capabilities))
		strcat(flags, "capture ");
	if (0 != (V4L2_CAP_VIDEO_OUTPUT & caps->capabilities))
		strcat(flags, "output ");
	if (0 != (V4L2_CAP_VIDEO_OVERLAY & caps->capabilities))
		strcat(flags, "overlay ");
	if (0 != (V4L2_CAP_VBI_CAPTURE & caps->capabilities))
		strcat(flags, "vbi_capture ");
	if (0 != (V4L2_CAP_VBI_OUTPUT & caps->capabilities))
		strcat(flags, "vbi_output ");
	if (0 != (V4L2_CAP_SLICED_VBI_CAPTURE & caps->capabilities))
		strcat(flags, "sliced_vbi_capture ");
	if (0 != (V4L2_CAP_SLICED_VBI_OUTPUT & caps->capabilities))
		strcat(flags, "sliced_vbi_output ");
	if (0 != (V4L2_CAP_RDS_CAPTURE & caps->capabilities))
		strcat(flags, "rds_capture ");
	if (0 != (V4L2_CAP_TUNER & caps->capabilities))
		strcat(flags, "tuner ");
	if (0 != (V4L2_CAP_AUDIO & caps->capabilities))
		strcat(flags, "audio ");
	if (0 != (V4L2_CAP_RADIO & caps->capabilities))
		strcat(flags, "radio ");
	if (0 != (V4L2_CAP_READWRITE & caps->capabilities))
		strcat(flags, "read_write ");
	if (0 != (V4L2_CAP_ASYNCIO & caps->capabilities))
		strcat(flags, "async_io ");
	if (0 != (V4L2_CAP_STREAMING & caps->capabilities))
		strcat(flags, "streaming ");

	LOGI("Video device capabilities...\n");
	LOGI("  Driver : %s\n", (char const*)caps->driver);
	LOGI("  Card   : %s\n", (char const*)caps->card);
	LOGI("  Bus    : %s\n", (char const*)caps->bus_info);
	LOGI("  Version: %u\n", caps->version);
	LOGI("  Flags  : %s\n", flags);
}

static void print_format_desc(struct v4l2_fmtdesc const *desc) {
	pixel_format_name_t name = { desc->pixelformat };

	LOGI("Format descriptor...\n");
	LOGI("  index       : %u\n", desc->index);
	LOGI("  flags       : %s\n", (0 != (V4L2_FMT_FLAG_COMPRESSED & desc->flags)) ? "compressed" : "none");
	LOGI("  description : %s\n", (char const*)desc->description);
	LOGI("  pixelformat : %c%c%c%c\n", name.name[0], name.name[1], name.name[2], name.name[3]);
}

static void print_pixel_format(struct v4l2_pix_format const *fmt) {
	pixel_format_name_t name = { fmt->pixelformat };
	LOGI("Pixel format...\n");
	LOGI("  width        : %u\n", fmt->width);
	LOGI("  height       : %u\n", fmt->height);
	LOGI("  pixelformat  : %c%c%c%c\n", name.name[0], name.name[1], name.name[2], name.name[3]);
	LOGI("  bytesperline : %u\n", fmt->bytesperline);
	LOGI("  sizeimage    : %u\n", fmt->sizeimage);
	LOGI("  colorspace   : %d\n", fmt->colorspace);
	LOGI("  private      : %u\n", fmt->priv);
}

static uint32_t to_v4l2_pixel_format(int format) {
	if (format >= UVCC_PIX_FMT_COUNT) {
		return PIXEL_FORMATS[DEF_PIXEL_FORMAT];
	}	
	return PIXEL_FORMATS[format];
}

static uint32_t from_v4l2_pixel_format(uint32_t format) {
	int i;
	for (i = 0; PIXEL_FORMATS[i]; ++i) {
		if (PIXEL_FORMATS[i] == format) {
			return i;
		}
	}
	return -1;
}

static int init_buffer(video_dev_t *dev) {
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	uint32_t i, count;
	video_buf_t *buf_ptr;
	int result = NOERROR;

	dev->buffers = NULL;
	dev->buffer_count = 0;

	memset(&req, 0, sizeof(req));
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (0 > ioctl(dev->fd, VIDIOC_REQBUFS, &req)) {
		if (EBUSY == errno) {
			LOGE("Buffer is already in progress.");
			return VIDEO_DEVICE_BUSY;
		}
		if (EINVAL == errno) {
			LOGE("Memory mapping is not supported.");
			return IO_METHOD_NOT_SUPPORTED;
		}
	}

	count = req.count;

	if (count < 2) {
		LOGE("Insufficient memory in video device driver.");
		return INSUFFICIENT_MEMORY;
	}

	buf_ptr = malloc(sizeof(video_buf_t) * count);
	if (NULL == buf_ptr) {
		LOGE("Insufficient memory in application.");
		return INSUFFICIENT_MEMORY;
	}

	for (i = 0; i < count; ++i) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.type = V4L2_MEMORY_MMAP;
		buf.index = i;

		buf_ptr[i].size = 0;
		buf_ptr[i].addr = MAP_FAILED;

		if (0 > ioctl(dev->fd, VIDIOC_QUERYBUF, &buf)) {
			if (EINVAL == errno) {
				break;
			} else {
				LOGE("Failed to query buffer (%s).", strerror(errno));
				result = VIDEO_DEVICE_QUERY_BUFFER_FAILED;
				break;
			}
		}

		buf_ptr[i].size = buf.length;
		buf_ptr[i].addr = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, dev->fd, buf.m.offset);

		if (MAP_FAILED == buf_ptr[i].addr) {
			LOGE("Failed to map the video memory (%s).", strerror(errno));
			result = MEMORY_MAPPING_FAILED;
			break;
		}
	}

	if (NOERROR != result) {
		for (i = 0; i < count; ++i) {
			if (MAP_FAILED == buf_ptr[i].addr) {
				break;
			}
			munmap(buf_ptr[i].addr, buf_ptr[i].size);
			buf_ptr[i].size = 0;
			buf_ptr[i].addr = NULL;
		}
		free(buf_ptr);
	} else {
		dev->buffers      = buf_ptr;
		dev->buffer_count = i;
	}

	return result;
}

int uvcc_open_video_device(uvcc_handle_t *handle, char const * const path) {
	video_dev_t *dev = NULL;
	uint32_t i;
	struct v4l2_fmtdesc desc;

	if (NULL == handle) {
		LOGE("'handle' parameter can not set to NULL.");
		return INVALID_ARGUMENTS;
	}
	if (NULL == path) {
		LOGE("'path' parameter can not set to NULL.");
		return INVALID_ARGUMENTS;
	}

	dev = (video_dev_t*)malloc(sizeof(video_dev_t));
	if (NULL == dev) {
		LOGE("Memory allocation failed.");
		return INSUFFICIENT_MEMORY;
	}

	memset(dev, 0, sizeof(video_dev_t));

	// set initial values.
	dev->fd = -1;
	dev->buffers = NULL;
	dev->buffer_count = 0;
	dev->is_capture_started = 0;

	dev->fd = open(path, O_RDONLY);
	if (dev->fd < 0) {
		LOGE("Can't open video devicie (%s).", path);
		if (EBUSY == errno) {
			LOGE("Vide device is busy.");
			return VIDEO_DEVICE_BUSY;
		}
		if (EPERM == errno) {
			LOGE("Operation not permitted.");
			return NOT_PERMITTED;
		}
		LOGE("Unknown error (%s).", strerror(errno));
		return VIDEO_DEVICE_OPEN_FAILED;
	}

	// get device capabilities
	if (0 > ioctl(dev->fd, VIDIOC_QUERYCAP, &dev->caps)) {
		LOGE("Video device capability can not get (%s).", strerror(errno));
		close(dev->fd);
		dev->fd = -1;
		return VIDEO_DEVICE_NOCAPS;
	}

	print_capability(&dev->caps);

	if (0 == (dev->caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		LOGE("Capture is not supported.");
		close(dev->fd);
		dev->fd = -1;
		return VIDEO_DEVICE_CAPTURE_NOT_SUPPORTED;
	}

	// get cropping capabilities
	memset(&dev->cropcaps, 0, sizeof(dev->cropcaps));
	dev->cropcaps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 > ioctl(dev->fd, VIDIOC_CROPCAP, &dev->cropcaps)) {
		LOGE("Video device crop capability can not get (%s).", strerror(errno));
		return VIDEO_DEVICE_NOCROPCAPS;
	}

	// enumerate pixel formats
	for (i = 0; ; ++i) {
		memset(&desc, 0, sizeof(desc));
		desc.index = i;
		desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (0 > ioctl(dev->fd, VIDIOC_ENUM_FMT, &desc)) {
			if (EINVAL == errno) {
				break;
			} else {
				LOGE("Failed to enumerate pixel formats (%s).", strerror(errno));
				return VIDEO_DEVICE_ENUM_FORMAT_FAILED;
			}
		}
		print_format_desc(&desc);
	}

	*handle = dev;

	return NOERROR;
}

void uvcc_close_video_device(uvcc_handle_t handle) {
	video_dev_t *dev = (video_dev_t*)handle;
	uint32_t i;

	if (NULL == dev) {
		return;
	}

	if (0 > dev->fd) {
		return;
	}

	if (NULL != dev->buffers) {
		for (i = 0; i < dev->buffer_count; ++i) {
			if (MAP_FAILED == dev->buffers[i].addr) {
				break;
			}
			munmap(dev->buffers[i].addr, dev->buffers[i].size);
		}
		free(dev->buffers);
	}

	int ret = -1;
	do {
		ret = close(dev->fd);
	} while (ret < 0);
	dev->fd = -1;
}

int uvcc_init_video_device(uvcc_handle_t handle, uint32_t width, uint32_t height, uint32_t pixel_format) {
	struct v4l2_format fmt;
	video_dev_t *dev = (video_dev_t*)handle;

	assert(NULL != dev);

	// set cropping area
	memset(&dev->crop, 0, sizeof(dev->crop));
	dev->crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dev->crop.c = dev->cropcaps.defrect;
	if (0 > ioctl(dev->fd, VIDIOC_S_CROP, &dev->crop)) {
		if (EINVAL == errno) {
			LOGW("Cropping is not supported.");
		} else {
			LOGE("Failed to set cropping area (%s).", strerror(errno));
			return VIDEO_DEVICE_CROPPING_FAILED;
		}
	}

	// set format
	memset(&dev->format, 0, sizeof(dev->format));
	dev->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dev->format.fmt.pix.width  = width;
	dev->format.fmt.pix.height = height;
	dev->format.fmt.pix.pixelformat = to_v4l2_pixel_format(pixel_format);
	dev->format.fmt.pix.field = V4L2_FIELD_INTERLACED;
	if (0 > ioctl(dev->fd, VIDIOC_S_FMT, &dev->format)) {
		if (EBUSY == errno) {
			LOGE("Video format can not be changed at this time.");
			return VIDEO_DEVICE_BUSY;
		}
		if (EINVAL == errno) {
			LOGE("Invalid format argument are set.");
			return INVALID_FORMAT_ARGUMENTS;
		}
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == ioctl(dev->fd, VIDIOC_G_FMT, &fmt)) {
		print_pixel_format(&fmt.fmt.pix);
		dev->format = fmt;
	}

	return init_buffer(dev);
}

int uvcc_start_capture(uvcc_handle_t handle) {
	video_dev_t *dev = (video_dev_t*)handle;
	struct v4l2_buffer buf;
	uint32_t i;
	uint32_t const count = (NULL == dev) ? 0 : dev->buffer_count;
	int retry;
	int result = NOERROR;
	enum v4l2_buf_type type;

	assert(NULL != dev);

	if (dev->is_capture_started) {
		return NOERROR;
	}

	for (i = 0; i < count; ++i) {
		for (retry = 0; retry < 5; ++retry) {
			memset(&buf, 0, sizeof(buf));
			buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index  = i;

			if (0 == ioctl(dev->fd, VIDIOC_QBUF, &buf)) {
				break;
			}

			switch (errno) {
			case EINVAL:
				LOGE("Non-blocking I/O has been selected and no buffer (index=%d).", i);
				result = MEMORY_QUEUEING_FAILED;
				break;
			case EIO:
				LOGE("Internal I/O error in video device.");
				result = IO_ERROR;
				break;
			case ENOMEM:
			case EAGAIN:
				usleep(10000);
				break;
			default:
				LOGE("Failed to queueing the buffer to start (%s, index=%d).", strerror(errno), i);
				result = MEMORY_QUEUEING_FAILED;
				break;
			}

			if (NOERROR != result) {
				break;
			}
		}

		if (retry >= 5) {
			LOGE("Retry failed.");
			result = MEMORY_QUEUEING_FAILED;
		}

		if (NOERROR != result) {
			break;
		}
	}

	if (NOERROR != result) {
		return result;
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 > ioctl(dev->fd, VIDIOC_STREAMON, &type)) {
		LOGE("Failed to start streaming (%s).", strerror(errno));
		result = VIDEO_DEVICE_STREAMING_FAILED;
	}

	if (NOERROR == result) {
		dev->is_capture_started = 1;
	}

	return result;
}

void uvcc_stop_capture(uvcc_handle_t handle) {
	enum v4l2_buf_type type;
	video_dev_t const *dev = (video_dev_t const*)handle;

	assert(NULL != dev);

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 > ioctl(dev->fd, VIDIOC_STREAMOFF, &type)) {
		LOGW("Failed to stop streaming (%s).", strerror(errno));
	}
}

int uvcc_capture(uvcc_handle_t handle, uint8_t * const buf, size_t buf_size) {
	video_dev_t const *dev = (video_dev_t const*)handle;
	int result;
	fd_set rfds;
	struct timeval tv;
	int n;

	assert(NULL != dev);

	if (!dev->is_capture_started) {
		result = uvcc_start_capture(handle);
		if (NOERROR != result) {
			return result;
		}
	}

	// capture!
	for (; ; ) {
		FD_ZERO(&rfds);
		FD_SET(dev->fd, &rfds);

		tv.tv_sec = 0;
		tv.tv_usec = 40000;

		n = select(dev->fd + 1, &rfds, NULL, NULL, &tv);

		if (n < 0) {
			if (ETIMEDOUT == errno) {
				continue;
			}
			LOGE("Failed to wait for capturable frame (%s).", strerror(errno));
			break;
		}

		if (!FD_ISSET(dev->fd, &rfds)) {
			continue;
		}

		result = read_frame(buf, buf_size, dev);
		break;
	}

	if (!dev->is_capture_started) {
		uvcc_stop_capture(handle);
	}

	return result;
}

uint32_t uvcc_get_frame_size(uvcc_handle_t handle) {
	video_dev_t const *dev = (video_dev_t const*)handle;
	if (NULL == dev) {
		return -1;
	}
	if ((0 == dev->buffer_count) || (NULL == dev->buffers)) {
		return -1;
	}
	return dev->buffers[0].size;
}

uint32_t uvcc_get_frame_width(uvcc_handle_t handle) {
	video_dev_t const *dev = (video_dev_t const*)handle;
	if (NULL == dev) {
		return -1;
	}
	return dev->format.fmt.pix.width;
}

uint32_t uvcc_get_frame_height(uvcc_handle_t handle) {
	video_dev_t const *dev = (video_dev_t const*)handle;
	if (NULL == dev) {
		return -1;
	}
	return dev->format.fmt.pix.height;
}

uint32_t uvcc_get_pixel_format(uvcc_handle_t handle) {
	video_dev_t const *dev = (video_dev_t const*)handle;
	if (NULL == dev) {
		return -1;
	}
	return from_v4l2_pixel_format(dev->format.fmt.pix.pixelformat);
}

