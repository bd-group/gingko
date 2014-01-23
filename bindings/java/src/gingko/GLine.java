package gingko;

import java.util.ArrayList;
import java.util.List;

public class GLine {
	
	public static class GLineHeader0 {
		short l1fld;
		short len;
		int llen;
	}
	
	public static class GLineHeader {
		short l1fld;
		short pad;
		int offset;
	}
	
	private GLineHeader0 lh0;
	private List<GLineHeader> lh = new ArrayList<GLineHeader>();
	private byte[] data;
	private int len;
	
	public GLine() {
	}

	public GLineHeader0 getLh0() {
		return lh0;
	}

	public void setLh0(GLineHeader0 lh0) {
		this.lh0 = lh0;
	}

	public List<GLineHeader> getLh() {
		return lh;
	}

	public void addLh(GLineHeader lh) {
		this.lh.add(lh);
	}

	public byte[] getData() {
		return data;
	}

	public void setData(byte[] data) {
		this.data = data;
	}

	public int getLen() {
		return len;
	}

	public void setLen(int len) {
		this.len = len;
	}
}
