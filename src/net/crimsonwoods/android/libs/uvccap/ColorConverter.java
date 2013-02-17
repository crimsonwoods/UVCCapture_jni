package net.crimsonwoods.android.libs.uvccap;

public class ColorConverter {
	static {
		System.loadLibrary("cconv");
	}
	public static native void yuyvtorgb(int[] rgba, byte[] yuyv, int width, int height);
}
