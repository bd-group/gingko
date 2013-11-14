/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-10-27 21:33:02 macan>
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

#ifndef __GINGKO_U_H__
#define __GINGKO_U_H__

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <signal.h>
#include <execinfo.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ucontext.h>
#include <regex.h>
#include <dlfcn.h>
#include <libgen.h>
#include "minilzo.h"
#include <search.h>

typedef unsigned long u64;
typedef signed long s64;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned short u16;
typedef signed short s16;

typedef unsigned char u8;
typedef signed char s8;

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_64

static inline u64 hash_64(u64 val, unsigned int bits)
{
	u64 hash = val;

	/*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
	u64 n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;

	/* High bits are more random, so use them. */
	return hash >> (64 - bits);
}

#include "err.h"
#include "xlist.h"
#include "atomic.h"

#define PATH_MAX 4096

#define xsleep usleep

#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);	\
	_x < _y ? _x : _y; })

#define max(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);	\
	_x > _y ? _x : _y; })

#define BITS_PER_LONG   64
/* for test_bit */
static inline int constant_test_bit(int nr, const volatile unsigned long *addr)
{
    return ((1UL << (nr % BITS_PER_LONG)) &
            (((unsigned long *)addr)[nr / BITS_PER_LONG])) != 0;
}

static inline int variable_test_bit(int nr, volatile const unsigned long *addr)
{
    int oldbit;

    /* sbb sub 1 more if CF is set, while bt set CF = bit value */
    asm volatile("bt %2,%1\n\t"
                 "sbb %0,%0"
                 : "=r" (oldbit)
                 : "m" (*(unsigned long *)addr), "Ir" (nr));
    
    return oldbit;
}

/* Note: test_bit() return 0 for 0, -1 for 1
 */
#define test_bit(nr,addr)                       \
    (__builtin_constant_p(nr) ?                 \
     constant_test_bit((nr),(addr)) :           \
     variable_test_bit((nr),(addr)))


#endif  /* !__GINGKO_U_H__ */
