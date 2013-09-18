/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-09-18 14:06:53 macan>
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

#ifndef __GINGKO_CONST_H__
#define __GINGKO_CONST_H__

#define MPCHECK_SENSITIVE_MAX   (5)
#define SECOND_IN_US            (1000000)
#define HALF_SECOND_IN_US       (500000) /* 1x faster */
#define QUAD_SECOND_IN_US       (250000) /* 2x */
#define EIGHTH_SECOND_IN_US     (125000) /* 4x */
#define SIXTEENTH_SECOND_IN_US  (62500)  /* 8x */
#define THIRTYND_SECOND_IN_US   (31250)  /* 16x */

static char *gingko_ccolor[] __attribute__((unused)) = 
{
    "\033[0;40;31m",            /* red */
    "\033[0;40;32m",            /* green */
    "\033[0;40;33m",            /* yellow */
    "\033[0;40;34m",            /* blue */
    "\033[0;40;35m",            /* pink */
    "\033[0;40;36m",            /* yank */
    "\033[0;40;37m",            /* white */
    "\033[0m",                  /* end */
};
#define GINGKO_COLOR(x)   (gingko_ccolor[x])
#define GINGKO_COLOR_RED  (gingko_ccolor[0])
#define GINGKO_COLOR_GREEN        (gingko_ccolor[1])
#define GINGKO_COLOR_YELLOW       (gingko_ccolor[2])
#define GINGKO_COLOR_BLUE         (gingko_ccolor[3])
#define GINGKO_COLOR_PINK         (gingko_ccolor[4])
#define GINGKO_COLOR_YANK         (gingko_ccolor[5])
#define GINGKO_COLOR_WHITE        (gingko_ccolor[6])
#define GINGKO_COLOR_END          (gingko_ccolor[7])

/* Define Error Number Here from 1025 */

#endif
