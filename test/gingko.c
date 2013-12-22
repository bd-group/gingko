/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-22 21:33:34 macan>
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

#include "gingko.h"

void __dump_f2p(struct field_2pack **fld, int fldnr)
{
    int i;

    for (i = 0; i < fldnr; i++) {
        printf("FLD %2d: type=%2d cidnr=%d data=%ld dlen=%d\n", i,
               fld[i]->type, fld[i]->cidnr, (long)fld[i]->data, fld[i]->dlen);
    }
}

int main(int argc, char *argv[])
{
    int err = 0, suidr, suidw;
    struct field schemas[7] = {
        {.name = "field1", .id = 0, .pid = FLD_MAX_PID, .type = GINGKO_INT64, 
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "field2", .id = 1, .pid = FLD_MAX_PID, .type = GINGKO_DOUBLE,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "field3", .id = 2, .pid = FLD_MAX_PID, .type = GINGKO_ARRAY,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "field4", .id = 3, .pid = 2, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "field5", .id = 4, .pid = FLD_MAX_PID, .type = GINGKO_STRUCT,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "field6", .id = 5, .pid = 4, .type = GINGKO_INT64,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "field7", .id = 6, .pid = 4, .type = GINGKO_DOUBLE,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
    };

    gingko_api_tracing_flags = 0xffffffff;
    gingko_su_tracing_flags = 0xffffffff;
    gingko_index_tracing_flags = 0xffffffff;
    
    err = gingko_init(NULL);
    if (err) {
        printf("gingko_init() failed w/ %d\n", err);
        goto out;
    }
    
    suidw = su_create(NULL, "./first_su", schemas, 7);
    if (suidw < 0) {
        printf("su_create() failed w/ %d\n", suidw);
        err = suidw;
        goto out;
    }
    printf("Create SU id=%d\n", suidw);

    suidr = su_open("./first_su", SU_OPEN_RDONLY);
    if (suidr < 0) {
        printf("su_open() failedd w/ %d\n", suidr);
        err = suidr;
        goto out;
    }
    printf("Open   SU id=%d\n", suidr);

    /* pack a data line */
    {
        struct field_2pack **flds = NULL, **t1, *tmp;
        double d = 20000.0;
        void *ts;
        struct line l;
        int fldnr = 0;

        /* INT64 */
        t1 = su_l1fieldpack(flds, &fldnr, su_new_field(GINGKO_INT64,
                                                       (void *)1000, 8));
        if (IS_ERR(t1)) {
            printf("su_l1fieldpack() failed w/ %s\n", gingko_strerror(PTR_ERR(t1)));
            goto out;
        }
        flds = t1;
        
        /* DOUBLE */
        memcpy(&ts, &d, sizeof(d));
        t1 = su_l1fieldpack(flds, &fldnr, su_new_field(GINGKO_DOUBLE,
                                                       ts, 8));
        if (IS_ERR(t1)) {
            printf("su_l1fieldpack() failed w/ %s\n", gingko_strerror(PTR_ERR(t1)));
            goto out;
        }
        
        /* ARRAY */
        tmp = su_new_field(GINGKO_ARRAY, NULL, 0);
        err = su_fieldpack(tmp, su_new_field(GINGKO_INT32, (void *)300, 4));
        if (err) {
            printf("su_field_pack() failed w/ %s\n", gingko_strerror(err));
            goto out;
        }
        err = su_fieldpack(tmp, su_new_field(GINGKO_INT32, (void *)400, 4));
        if (err) {
            printf("su_field_pack() failed w/ %s\n", gingko_strerror(err));
            goto out;
        }
        err = su_fieldpack(tmp, su_new_field(GINGKO_INT32, (void *)500, 4));
        if (err) {
            printf("su_field_pack() failed w/ %s\n", gingko_strerror(err));
            goto out;
        }
        t1 = su_l1fieldpack(flds, &fldnr, tmp);
        if (IS_ERR(t1)) {
            printf("su_l1fieldpack() failed w/ %s\n", gingko_strerror(PTR_ERR(t1)));
            goto out;
        }
        flds = t1;

        /* STRUCT */
        tmp = su_new_field(GINGKO_STRUCT, NULL, 0);
        err = su_fieldpack(tmp, su_new_field(GINGKO_INT64, (void *)5000, 8));
        if (err) {
            printf("su_field_pack() failed w/ %s\n", gingko_strerror(err));
            goto out;
        }
        d = 90000.0;
        memcpy(&ts, &d, sizeof(d));
        err = su_fieldpack(tmp, su_new_field(GINGKO_DOUBLE, ts, 8));
        if (err) {
            printf("su_field_pack() failed w/ %s\n", gingko_strerror(err));
            goto out;
        }
        t1 = su_l1fieldpack(flds, &fldnr, tmp);
        if (IS_ERR(t1)) {
            printf("su_l1fieldpack() failed w/ %s\n", gingko_strerror(PTR_ERR(t1)));
            goto out;
        }
        flds = t1;
        __dump_f2p(flds, fldnr);

        memset(&l, 0, sizeof(l));
        su_linepack(&l, flds, 4);

        err = su_write(suidw, &l, 0);
        if (err) {
            printf("su_write(%d) failed w/ %s\n", suidw, gingko_strerror(err));
            goto out;
        }

        err = su_write(suidw, &l, 1);
        if (err) {
            printf("su_write(%d) failed w/ %s\n", suidw, gingko_strerror(err));
            goto out;
        }
    }
    
    su_close(suidw);
    su_close(suidr);

out:
    return err;
}
