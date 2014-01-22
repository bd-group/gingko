/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-22 09:20:33 macan>
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

int __gen_dx_rz(int suidw, struct field schemas[], int schlen, int lnr, long *wb)
{
    int i, j, err = 0;

    *wb = 0;
    for (i = 0; i < lnr; i++) {
        struct field_2pack **flds = NULL, **t1;
        struct line l;
        int fldnr = 0;
        
        for (j = 0; j < schlen; j++) {
            switch (schemas[j].type) {
            case GINGKO_INT64:
                t1 = su_l1fieldpack(flds, &fldnr, su_new_field(GINGKO_INT64,
                                                               (void *)(long)j, 8));
                if (IS_ERR(t1)) {
                    printf("su_l1fieldpack() failed w/ %s\n", 
                           gingko_strerror(PTR_ERR(t1)));
                    goto out;
                }
                flds = t1;
                *wb += 8;
                break;
            case GINGKO_INT32:
                t1 = su_l1fieldpack(flds, &fldnr, su_new_field(GINGKO_INT32,
                                                               (void *)(long)j, 4));
                if (IS_ERR(t1)) {
                    printf("su_l1fieldpack() failed w/ %s\n", 
                           gingko_strerror(PTR_ERR(t1)));
                    goto out;
                }
                flds = t1;
                *wb += 4;
                break;
            case GINGKO_STRING:
            {
                char *str = xmalloc(256);

                if (!str) {
                    printf("alloc str failed, no memory\n");
                    goto out;
                }
                sprintf(str, "%s=%d.%d.%d.%d", schemas[j].name,
                        i % 256, i % 256, i % 256, i % 256);
                t1 = su_l1fieldpack(flds, &fldnr, su_new_field(GINGKO_STRING, 
                                                               (void *)str, strlen(str)));
                if (IS_ERR(t1)) {
                    printf("su_l1fieldpack() failed w/ %s\n", 
                           gingko_strerror(PTR_ERR(t1)));
                    goto out;
                }
                flds = t1;
                *wb += strlen(str);
                break;
            }
            }
        }

        //__dump_f2p(flds, fldnr);
    
        memset(&l, 0, sizeof(l));
        su_linepack(&l, flds, fldnr);
        su_free_field_2pack(flds, fldnr);
        xfree(flds);

#if 0
        printf("Write Data Line %d ...\n", i);
#endif
        err = su_write(suidw, &l, i);
        if (err) {
            printf("su_write(%d) failed w/ %s\n", suidw, gingko_strerror(err));
            goto out;
        }
        xfree(l.data);
        xfree(l.lh);
    }

    printf("Total Written Data Line %d ...\n", lnr);
    
out:
    return err;
}

int __do_write(struct field *schemas, int schlen, char *supath, int wnr)
{
    int err = 0, suidw;
    struct su_conf sc = {
        .page_size = SU_PAGE_SIZE,
        /* .page_size = 2 * 1024, */
        .page_algo = SU_PH_COMP_LZ4,
    };
    struct timeval begin, end;
    long wb;

    suidw = su_create(&sc, "./first_su", schemas, schlen);
    if (suidw < 0) {
        printf("su_create() failed w/ %d\n", suidw);
        err = suidw;
        goto out;
    }
    printf("Create SU id=%d\n", suidw);

    gettimeofday(&begin, NULL);
    err = __gen_dx_rz(suidw, schemas, schlen, wnr, &wb);
    if (err) {
        printf("__gen_dx_rz() failed w/ %d\n", err);
        goto out_close;
    }
    gettimeofday(&end, NULL);
    printf("[WR] DX_RZ WBytes=%ld Rate=%lf BW=%lf KB/s\n",
           wb, 
           wnr / ((end.tv_sec - begin.tv_sec) 
                  + (end.tv_usec - begin.tv_usec) / 1000000.0),
           wb / ((end.tv_sec - begin.tv_sec) 
                 + (end.tv_usec - begin.tv_usec) / 1000000.0) / 1024);

out_close:
    su_close(suidw);
out:
    return err;
}

int __read_dx_rz(int suidr, int lids[], int lnr, long *rb)
{
    struct field_g fields[] = {
        {.id = 0, .flags = UNPACK_FNAME,},
        {.id = 8, .flags = UNPACK_FNAME,},
        {.id = 6, .flags = UNPACK_FNAME,},
        {.id = FLD_MAX_PID, .name = "c_ydz"},
        {.id = FLD_MAX_PID, .name = "c_ydzhdlx"},
        {.id = 5, .flags = UNPACK_FNAME,},
        {.id = 15, .flags = UNPACK_FNAME,},
/*
        {.id = 16, .flags = UNPACK_FNAME,},
        {.id = 17, .flags = UNPACK_FNAME,},
        {.id = 18, .flags = UNPACK_FNAME,},
        {.id = 21, },
        {.id = 22, },
        {.id = 23, },
        {.id = 24, },
        {.id = 25, },
        {.id = 26, },
        {.id = 27, },
        {.id = 37, },
        {.id = 38, },
        {.id = 39, },
        {.id = 40, },
        {.id = 41, },
*/
        {.id = 42},
    };
    int i, j, err = 0;
    
    *rb = 0;
    for (i = 0; i < lnr; i++) {
        char str[256];
        
        err = su_get(suidr, lids[i], fields, 
                     sizeof(fields) / sizeof(struct field_g));
        if (err) {
            printf("su_get() failed w/ %d\n", err);
            goto out;
        }
        for (j = 0; j < sizeof(fields) / sizeof(struct field_g); j++) {
            *rb += fields[j].dlen;
            if (fields[j].type == GINGKO_STRING) {
#if 0
                printf("LID=%d IDX=%d -> {id=%d pid=%d type=%s dlen=%d name=%s "
                       "content=[%.*s]}\n",
                       lids[i], j,
                       fields[j].id,
                       fields[j].pid,
                       gingko_type(fields[j].type),
                       fields[j].dlen,
                       fields[j].name,
                       fields[j].dlen,
                       (char *)fields[j].content);
#endif
                sprintf(str, "%s=%d.%d.%d.%d", fields[j].name,
                        i % 256, i % 256, i % 256, i % 256);
                if (strncmp(str, fields[j].content, fields[j].dlen) != 0) {
                    err = -EINTERNAL;
                }
                xfree(fields[j].content);
            } else {
#if 0
                int *pi = (int *)&fields[j].content;
                double *pd = (double *)&fields[j].content;

                printf("LID=%d IDX=%d -> {id=%d pid=%d type=%s dlen=%d name=%s "
                       "content=[%d|%ld|%lf]}\n",
                       lids[i], j,
                       fields[j].id,
                       fields[j].pid,
                       gingko_type(fields[j].type),
                       fields[j].dlen,
                       fields[j].name,
                       *pi,
                       (u64)fields[j].content,
                       *pd);
#endif
                if (fields[j].type == GINGKO_INT64) {
                    long *pl = (long *)&fields[j].content;
                    if (*pl != fields[j].id)
                        err = -EINTERNAL;
                } else if (fields[j].type == GINGKO_INT32) {
                    int *pi = (int *)&fields[j].content;
                    if (*pi != fields[j].id)
                        err = -EINTERNAL;
                }
            }
            if (fields[j].flags == UNPACK_ALL ||
                fields[j].flags == UNPACK_FNAME) {
                xfree(fields[j].name);
                fields[j].name = NULL;
            }
            fields[j].content = NULL;
        }
    }

out:
    return err;
}

int __do_read(char *supath, int rnr)
{
    int err = 0, suidr;
    int *lids, i;
    struct timeval begin, end;
    long rb;

    lids = xmalloc(sizeof(int) * rnr);
    if (!lids) {
        return 0;
    }
    for (i = 0; i < rnr; i++) {
        lids[i] = i;
    }
    
    //gingko_su_tracing_flags = GINGKO_ERR | GINGKO_WARN | GINGKO_INFO | GINGKO_DEBUG;
    gingko_su_tracing_flags = GINGKO_ERR | GINGKO_WARN | GINGKO_INFO;
    suidr = su_open("./first_su", SU_OPEN_RDONLY, NULL);
    gingko_su_tracing_flags = GINGKO_ERR | GINGKO_WARN;
    if (suidr < 0) {
        printf("su_open() failed w/ %d\n", suidr);
        err = suidr;
        goto out;
    }
    printf("Open   SU id=%d\n", suidr);

    gettimeofday(&begin, NULL);
    err = __read_dx_rz(suidr, lids, rnr, &rb);
    if (err) {
        printf("__read_dx_rz() failed w/ %d\n", err);
        goto out_close;
    }
    gettimeofday(&end, NULL);
    printf("[RD] DX_RZ RBytes=%ld Rate=%lf BW=%lf KB/s\n",
           rb, 
           rnr / ((end.tv_sec - begin.tv_sec) 
                  + (end.tv_usec - begin.tv_usec) / 1000000.0),
           rb / ((end.tv_sec - begin.tv_sec) 
                 + (end.tv_usec - begin.tv_usec) / 1000000.0) / 1024);
    
out_close:
    su_close(suidr);
out:
    return err;
}

int main(int argc, char *argv[])
{
    int err = 0;
    struct field schemas[44] = {
        {.name = "c_dxxh", .id = 0, .pid = FLD_MAX_PID, .type = GINGKO_INT64, 
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_yysip", .id = 1, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_jrdid", .id = 2, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz", .id = 3, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzhdlx", .id = 4, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzhd", .id = 5, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzyysid", .id = 6, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzsssid", .id = 7, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzssqh", .id = 8, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzdqsid", .id = 9, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzdqqh", .id = 10, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzdqjz", .id = 11, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydzsphm", .id = 12, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddz", .id = 13, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzhdlx", .id = 14, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzhd", .id = 15, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzyysid", .id = 16, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzsssid", .id = 17, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzssqh", .id = 18, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzdqsid", .id = 19, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzdqqh", .id = 20, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_mddzsphm", .id = 21, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_sfddd", .id = 22, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_sfjscl", .id = 23, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_fssj", .id = 24, .pid = FLD_MAX_PID, .type = GINGKO_INT64,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_sfccdx", .id = 25, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ccdxzts", .id = 26, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ccdxxh", .id = 27, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ccdxxlh", .id = 28, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_bmlx", .id = 29, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_dxnr", .id = 30, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_nrzymd5", .id = 31, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_esmnr", .id = 32, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_fdj_ip", .id = 33, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_sfgzbmd", .id = 34, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_imsi", .id = 35, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_imei", .id = 36, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_spcode", .id = 37, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_ascode", .id = 38, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_lac", .id = 39, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_ci", .id = 40, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_rac", .id = 41, .pid = FLD_MAX_PID, .type = GINGKO_INT32,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_areacode", .id = 42, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
        {.name = "c_ydz_homecode", .id = 43, .pid = FLD_MAX_PID, .type = GINGKO_STRING,
         .codec = FLD_CODEC_NONE, .cidnr = 0},
    };

    gingko_api_tracing_flags = GINGKO_ERR | GINGKO_WARN;
    gingko_su_tracing_flags = GINGKO_ERR | GINGKO_WARN;
    gingko_index_tracing_flags = GINGKO_ERR | GINGKO_WARN;
    
    struct gingko_conf gc = {
        .max_suid = GINGKO_MAX_SUID,
        .gsrh_size = GINGKO_GSRH_SIZE,
        .pcrh_size = GINGKO_PCRH_SIZE,
        .pc_memory = 1024 * 1024 * 1024,
    };

    err = gingko_init(&gc);
    if (err) {
        printf("gingko_init() failed w/ %d\n", err);
        goto out;
    }
    if (argc > 2) {
        if (strcmp(argv[1], "w") == 0) {
            err = __do_write(schemas, 44, "./first_su", atoi(argv[2]));
            if (err) {
                printf("__do_write() failed w/ %d\n", err);
                goto out_fina;
            }
        } else if (strcmp(argv[1], "r") == 0) {
            err = __do_read("./first_su", atoi(argv[2]));
            if (err) {
                printf("__do_read() failed w/ %d\n", err);
                goto out_fina;
            }
        } else if (strcmp(argv[1], "rw") == 0) {
            err = __do_write(schemas, 44, "./first_su", atoi(argv[2]));
            if (err) {
                printf("__do_write() failed w/ %d\n", err);
                goto out_fina;
            }
            err = __do_read("./first_su", atoi(argv[2]));
            if (err) {
                printf("__do_read() failed w/ %d\n", err);
                goto out_fina;
            }
        }
    }

out_fina:
    err = gingko_fina();
    if (err) {
        printf("gingko_fina() failed w/ %d\n", err);
        goto out;
    }

out:
    return err;
}
