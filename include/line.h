/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-20 16:33:50 macan>
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

#ifndef __LINE_H__
#define __LINE_H__

struct lineheader0
{
    u16 l1fld;   //L 1(num) F L D 
    u16 len;
    u32 llen;    // LLen
};

struct lineheader
{
    u16 l1fld;
    u16 pad;
    u32 offset;  //offset in current page
};

struct line
{
    struct lineheader *lh;  //line header
    void *data;             //line data
    int len;
};

extern struct lineheader lharray[]; //total 1 + lharray[0].len

struct column 
{
    u16 fld;                    /* field ID */
    u16 flag;                   /* which light-weight codec */
    void *codec;                /* refer to light-weight codec */
    void *fldstat;              /* refer to field statistic area */
};

#endif
