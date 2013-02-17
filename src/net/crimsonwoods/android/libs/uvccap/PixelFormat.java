package net.crimsonwoods.android.libs.uvccap;

public enum PixelFormat{
	RGB565(0),
	RGB32(1),
	BGR32(2),
	YUYV(3),
	UYVY(4),
	YUV420(5),
	YUV410(6),
	YUV422P(7),
	NV12(8),
	NV21(9),
	UNKNOWN(-1);
	
	int value;
	
	PixelFormat(int value) {
		this.value = value;
	}
	
	public static PixelFormat from(int value) {
		switch(value) {
		case 0:
			return RGB565;
		case 1:
			return RGB32;
		case 2:
			return BGR32;
		case 3:
			return YUYV;
		case 4:
			return UYVY;
		case 6:
			return YUV410;
		case 7:
			return YUV422P;
		case 8:
			return NV12;
		case 9:
			return NV21;
		default:
			return UNKNOWN;
		}
	}
}