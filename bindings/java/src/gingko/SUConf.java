package gingko;

public class SUConf {
	
	public static class SU_CONF {
		public static final int SU_PAGE_SIZE = (128 * 1024);
		public static final int SU_PAGE_SIZE_MAX = (64 * 1024 * 1024);
		public static final int SU_FC_SIZE = (256);
	}
	
	private int page_size;
	private int page_algo;
	private int fc_size;
	
	public SUConf(int page_size, int page_algo, int fc_size) {
		this.setPage_size(page_size);
		this.setPage_algo(page_algo);
		this.setFc_size(fc_size);
	}

	public int getPage_algo() {
		return page_algo;
	}

	public void setPage_algo(int page_algo) {
		this.page_algo = page_algo;
	}

	public int getPage_size() {
		return page_size;
	}

	public void setPage_size(int page_size) {
		this.page_size = page_size;
	}

	public int getFc_size() {
		return fc_size;
	}

	public void setFc_size(int fc_size) {
		this.fc_size = fc_size;
	}
}
