package net.crimsonwoods.android.libs.uvccap;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class UVCCamera {
	private static final String DEVICE_PATH_PREFIX = "/dev/video";
	private boolean isStarted = false;
	private long nativeHandle = 0;
	private final String devicePath;
	
	static {
		System.loadLibrary("uvccap");
	}
	
	private UVCCamera(String device) {
		devicePath = device;
	}
	
	public static synchronized UVCCamera open(String devicePath) throws IOException {
		final UVCCamera camera = new UVCCamera(devicePath);
		camera.open();
		return camera;
	}
	
	@Override
	protected void finalize() throws Throwable {
		release();
	}
	
	private synchronized void open() throws IOException {
		nativeHandle = n_open(devicePath);
	}
	
	public synchronized void init(int width, int height, PixelFormat format) throws IOException {
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
	
	public List<FrameSize> getSupportedPreviewSizes(PixelFormat format) {
		ArrayList<FrameSize> frameSizes = new ArrayList<FrameSize>();
		for (int index = 0; ; ++index) {
			final FrameSize size = n_enumFrameSize(nativeHandle, index, format.value);
			if (null == size) {
				break;
			}
			frameSizes.add(size);
		}
		return frameSizes;
	}
	
	public static final class FrameSize {
		public int width;
		public int height;
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
	private native FrameSize n_enumFrameSize(long handle, int index, int pixelFormat);
}
