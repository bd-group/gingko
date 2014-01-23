package gingko;

import java.util.ArrayList;
import java.util.List;

public class GField_2pack {
	private byte type;
	private int cidnr;
	private byte[] data;
	private int dlen;
	private List<GField_2pack> clds = new ArrayList<GField_2pack>();
	
	public GField_2pack(byte type, int cidnr, byte[] data, int dlen, List<GField_2pack> clds) {
		this.setType(type);
		this.setCidnr(cidnr);
		this.setData(data);
		this.setDlen(dlen);
		if (clds != null)
			this.clds.addAll(clds);
	}

	public byte getType() {
		return type;
	}

	public void setType(byte type) {
		this.type = type;
	}

	public int getCidnr() {
		return cidnr;
	}

	public void setCidnr(int cidnr) {
		this.cidnr = cidnr;
	}

	public byte[] getData() {
		return data;
	}

	public void setData(byte[] data) {
		this.data = data;
	}

	public int getDlen() {
		return dlen;
	}

	public void setDlen(int dlen) {
		this.dlen = dlen;
	}
}
