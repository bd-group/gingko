package gingko;

public class Gingko {

	static {
		System.loadLibrary("libgk");
		System.loadLibrary("libgk4j");
	}
	
	public static native int gingko_init(GingkoConf conf);
	
	public static native int gingko_fina();
	
	public static native int su_open(String supath, int mode, GingkoConf conf);
	
	public static native int su_create(SUConf sc, String supath, GField schemas[]);
	
	public static native int su_linepack(GLine line, GField_2pack flds[]);
	
	public static native int su_fieldpack(GField_2pack parent, GField_2pack child);
	
	public static native int su_l1fieldpack(GField_2pack old[], GField_2pack ngf);
	
	public static native GField_2pack su_new_field(byte type, byte[] data, int dlen);
	
	public static native void su_free_field_2pack(GField_2pack flds[]);
	
	public static native int su_write(int suid, GLine line, long lid);
	
	public static native int su_sync(int suid);
	
	public static native int su_close(int suid);
	
	public static native int su_get(int suid, long lid, GField_g fields[]);

}