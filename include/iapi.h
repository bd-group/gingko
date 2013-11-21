/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-20 16:40:52 macan>
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

/* APIs */
int df_read_meta(struct gingko_su *gs, struct dfile *df);
int df_write_meta(struct gingko_su *gs, struct dfile *df, int write_schema);
int df_write_l2p(struct gingko_su *gs, struct dfile *df);

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

struct field_t *alloc_field_t(int type);
int linepack_primitive(struct line *line, void *data, int dlen);
int linepack_string(struct line *line, void *data, int dlen);
int linepack_complex(struct line *line, struct field_2pack *fld);

#endif
