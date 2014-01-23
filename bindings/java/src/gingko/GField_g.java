package gingko;

public class GField_g {
	
	public static class UNPACK_FLAG {
		public static final byte UNPACK_ALL = 0;
		public static final byte UNPACK_DATAONLY = 1;
		public static final byte UNPACK_FNAME = 2;
	}
	private short id;
	private short orig_id;
	private short pid;
	private byte type;
	private byte flags;
	private String name;
	private byte[] content;
	private int dlen;
	
	public GField_g(short id) {
		this.setId(this.setOrig_id(id));
	}
	
	public GField_g(String name) {
		this.setName(name);
	}

	public short getId() {
		return id;
	}

	public void setId(short id) {
		this.id = id;
	}

	public short getOrig_id() {
		return orig_id;
	}

	public short setOrig_id(short orig_id) {
		this.orig_id = orig_id;
		return orig_id;
	}

	public short getPid() {
		return pid;
	}

	public void setPid(short pid) {
		this.pid = pid;
	}

	public byte getType() {
		return type;
	}

	public void setType(byte type) {
		this.type = type;
	}

	public byte getFlags() {
		return flags;
	}

	public void setFlags(byte flags) {
		this.flags = flags;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public byte[] getContent() {
		return content;
	}

	public void setContent(byte[] content) {
		this.content = content;
	}

	public int getDlen() {
		return dlen;
	}

	public void setDlen(int dlen) {
		this.dlen = dlen;
	}
}
