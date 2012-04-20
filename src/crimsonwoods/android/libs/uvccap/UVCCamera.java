package crimsonwoods.android.libs.uvccap;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class UVCCamera {
	private static final String DEVICE_PATH_PREFIX = "/dev/video";
	private boolean isStarted = false;
	private long nativeHandle = 0;
	private final String devicePath;
	private static final Map<Integer, UVCCamera> instanceMap = new HashMap<Integer, UVCCamera>();
	
	static {
		System.loadLibrary("uvccap");
	}
	
	private UVCCamera(String device) {
		devicePath = device;
	}
	
	public static UVCCamera open() {
		return open(0);
	}
	
	public static synchronized UVCCamera open(int index) {
		if (0 > index) {
			throw new IllegalArgumentException("'index' have to be set to zero or positive value.");
		}
		if (!instanceMap.containsKey(index)) {
			instanceMap.put(index, new UVCCamera(DEVICE_PATH_PREFIX + index));
		}
		return instanceMap.get(index);
	}
	
	@Override
	protected void finalize() throws Throwable {
		release();
	}
	
	public synchronized void init(int width, int height, PixelFormat format) throws IOException {
		nativeHandle = n_open(devicePath);
		n_init(nativeHandle, width, height, format.value);
	}
	
	public synchronized void release() {
		if (isStarted) {
			n_stop(nativeHandle);
			isStarted = false;
		}
		n_close(nativeHandle);
		nativeHandle = 0;
	}
	
	public synchronized void capture(byte[] pixels) {
		if (!isStarted) {
			n_start(nativeHandle);
			isStarted = true;
		}
		n_capture(nativeHandle, pixels);
	}
	
	public synchronized PixelFormat getPixelFormat() {
		return PixelFormat.from(n_getPixelFormat(nativeHandle));
	}
	
	public synchronized int getFrameSize() {
		return n_getFrameSize(nativeHandle);
	}
	
	public synchronized int getWidth() {
		return n_getWidth(nativeHandle);
	}
	
	public synchronized int getHeight() {
		return n_getHeight(nativeHandle);
	}
	
	private static final int fourcc(byte[] seq) {
		if (seq.length != 4) {
			throw new IllegalArgumentException();
		}
		return (seq[0] << 24) | (seq[1] << 16) | (seq[2] << 8) | seq[3];
	}
	
	private native int n_getPixelFormat(long handle);
	private native int n_getFrameSize(long handle);
	private native int n_getWidth(long handle);
	private native int n_getHeight(long handle);
	private native long n_open(String device) throws IOException;
	private native void n_init(long handle, int width, int height, int pixelFormat) throws IOException;
	private native void n_close(long handle);
	private native void n_capture(long handle, byte[] pixels);
	private native void n_start(long handle);
	private native void n_stop(long handle);
}
