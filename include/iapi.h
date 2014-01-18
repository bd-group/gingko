/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-18 22:08:48 macan>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __IAPI_H__
#define __IAPI_H__

extern struct gingko_manager g_mgr;

/* APIs */
int df_read_meta(struct gingko_su *gs, struct dfile *df);
int df_write_meta(struct gingko_su *gs, struct dfile *df, int write_schema);
int df_write_l2p(struct gingko_su *gs, struct dfile *df);
int df_append_l2p(struct gingko_su *gs, struct dfile *df);
int df_read_l2p(struct gingko_su *gs, struct dfile *df);
u64 __l2p_lookup_pgoff(struct gingko_su *gs, struct dfile *df, long lid);

int su_read_meta(struct gingko_su *gs);
int su_write_meta(struct gingko_su *gs);

int alloc_dfile(struct dfile *df);
int init_dfile(struct gingko_su *gs, struct field schemas[], int schelen, 
               struct dfile *df);
void fina_dfile(struct dfile *df);

int verify_schema(struct gingko_su *gs, struct field schemas[], int schelen);
typedef void (*tfunc)(void *arg);
void __pre_trav_schemas(struct field_t *root, tfunc cb);
void __post_trav_schemas(struct field_t *root, tfunc cb);
void __free_schema(void *arg);

struct field_t *__find_field_t_by_name(struct field_t *root, char *name);
struct field_t *__find_field_t(struct field_t *root, int id);

struct field_t *alloc_field_t(int type);
int linepack_primitive(struct line *line, void *data, int dlen);
int linepack_string(struct line *line, void *data, int dlen);
int linepack_complex(struct line *line, struct field_2pack *fld);
int lineunpack(void *idata, int dlen, struct field_t *f, struct field_g *fg);
int lineparse(struct field_g *f, struct field_t *this, struct field_t *tf);
int build_lineheaders(struct gingko_su *gs, struct line *line, long lid,
                      u64 coff);
void dump_lineheader(long lid, struct line *line);

struct gingko_conf;
int pagecache_init(struct gingko_conf *conf);
int pagecache_fina(void);
int pagecache_memlimit(int new_size);
void pagecache_raise_memlimit(int size);
void __pc_lru_update(struct page *p);
void __pc_lru_remove(struct page *p);
void __pcrh_remove(struct page *p);
int __pcrh_insert(struct page *p);
struct page *__pcrh_lookup(char *suname, int dfid, u64 pgoff);
struct page *get_page(struct gingko_su *gs, int dfid, u64 pgoff);
void put_page(struct page *p);

int page_write(struct page *p, struct line *line, long lid, 
               struct gingko_su *gs);
void put_page(struct page *p);
void dump_page(struct page *p);
int page_sync(struct page *p, struct gingko_su *gs);
struct page *page_load(struct gingko_su *gs, int dfid, u64 pgoff);
void async_page_sync(struct page *p, struct gingko_su *gs);

int page_read(struct gingko_su *gs, struct page *p, long lid, 
              struct field_g fields[], int fldnr, int flag);

#define SU_PAGE_ALLOC_ADDL2P    0x01
#define SU_PAGE_ALLOC_NOINSERT  0x02
struct page *__alloc_page(struct gingko_su *gs, int dfid, int flags);
void __free_page(struct page *p);

int build_pageindex(struct page *p, struct line *line, long lid,
                    struct gingko_su *gs);
void dump_pageindex(struct pageindex *pi);

#endif
