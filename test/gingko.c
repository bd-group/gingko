/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-17 19:35:01 macan>
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

int main(int argc, char *argv[])
{
    int err = 0, suid;
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
    
    err = gingko_init(NULL);
    if (err) {
        printf("gingko_init() failed w/ %d\n", err);
        goto out;
    }
    
    suid = su_create("./first_su", schemas, 7);
    if (suid < 0) {
        printf("su_create() failed w/ %d\n", suid);
        err = suid;
        goto out;
    }
    printf("Create SU id=%d\n", suid);
    su_close(suid);

    suid = su_open("./first_su", SU_OPEN_RDONLY);
    if (suid < 0) {
        printf("su_open() failedd w/ %d\n", suid);
        err = suid;
        goto out;
    }
    printf("Open   SU id=%d\n", suid);

    su_close(suid);

out:
    return err;
}
