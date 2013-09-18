/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-09-18 14:21:29 macan>
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

#include "lib.h"

#ifdef GINGKO_TRACING
//u32 gingko_lib_tracing_flags = GINGKO_DEFAULT_LEVEL | GINGKO_DEBUG_ALL;
u32 gingko_lib_tracing_flags = GINGKO_DEFAULT_LEVEL;
#endif

#ifdef GINGKO_DEBUG_LOCK
struct list_head glt;           /* global lock table */
#endif

u64 cpu_frequency = 0;

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#define BT_SIZE 50
#define BT_FLEN 512

struct backtrace_info
{
    void *bt[BT_SIZE];
    int size;
};

static struct backtrace_info bi;

void lib_init(void)
{
    FILE *fp;
    
    srandom(time(NULL));
    /* get the cpu frequency */
    fp = popen("cat /proc/cpuinfo | grep 'cpu MHz' | "
               "awk '{print $4}' | head -n1", "r");
    if (!fp) {
        gingko_err(lib, "get cpu frequency failed w/ %s(%d)\n",
                 strerror(errno), errno);
        return;
    }
    if (fscanf(fp, "%ld", &cpu_frequency) < 0)
        return;
    pclose(fp);
    if (!cpu_frequency)
        cpu_frequency = 2000;
    gingko_info(lib, "Detect CPU frequency: %ld MHz\n", cpu_frequency);
    cpu_frequency *= 1024 * 1024;
}

/* do backtrace
 */
void lib_backtrace(void)
{
    char **bts;
    char cmd[BT_FLEN] = "addr2line -e ";
    char *prog = cmd + strlen(cmd);
    char str[BT_FLEN];
    FILE *fp;
    int i;

    int r = readlink("/proc/self/exe", prog, sizeof(cmd) -
                     (prog - cmd) - 1);

    memset(&bi, 0, sizeof(bi));
    bi.size = backtrace(bi.bt, BT_SIZE);
    if (bi.size > 0) {
        bts = backtrace_symbols(bi.bt, bi.size);
        if (bts) {
            for (i = 1; i < bi.size; i++) {
                if (!r) {
                    sprintf(str, "%s %p", cmd, bi.bt[i]);
                    fp = popen(str, "r");
                    if (!fp) {
                        gingko_info(lib, "%s\n", bts[i]);
                        continue;
                    } else {
                        if (fscanf(fp, "%s", str) > 0)
                            gingko_info(lib, "%s %s\n", bts[i], str);
                        else
                            gingko_info(lib, "%s %s\n", bts[i], "Unknown");
                    }
                    pclose(fp);
                } else {
                    gingko_info(lib, "%s\n", bts[i]);
                }
            }
            free(bts);
        }
    }
}
