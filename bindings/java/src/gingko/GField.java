package gingko;

public class GField {
	
	public static class GINGKO_TYPE {
		public static final byte GINGKO_INT8 = 1;
		public static final byte GINGKO_INT16 = 2;
		public static final byte GINGKO_INT32 = 3;
		public static final byte GINGKO_INT64 = 4;
		public static final byte GINGKO_FLOAT = 5;
		public static final byte GINGKO_DOUBLE = 6;
		public static final byte GINGKO_STRING = 7;
		public static final byte GINGKO_BYTES = 8;
		public static final byte GINGKO_ARRAY = 9;
		public static final byte GINGKO_STRUCT = 10;
		public static final byte GINGKO_MAP = 11;
	}
	
	public static class FLD_CODEC {
		public static final byte FLD_CODEC_NONE = 0;
		public static final byte FLD_CODEC_RUNLENGTH = 1;
		public static final byte FLD_CODEC_DELTA = 2;
		public static final byte FLD_CODEC_DICT = 3;
	}
	
	private String name;
	private short id;
	private byte type;
	private byte codec;
	private int cidnr;
	
	public GField(String name, short id, byte type, byte codec) {
		this.name = name;
		this.id = id;
		this.type = type;
		this.codec = codec;
	}
	
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public short getId() {
		return id;
	}
	public void setId(short id) {
		this.id = id;
	}
	public byte getType() {
		return type;
	}
	public void setType(byte type) {
		this.type = type;
	}
	public byte getCodec() {
		return codec;
	}
	public void setCodec(byte codec) {
		this.codec = codec;
	}
	public int getCidnr() {
		return cidnr;
	}
	public void setCidnr(int cidnr) {
		this.cidnr = cidnr;
	}

}
