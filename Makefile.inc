##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2013-09-18 15:20:40 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

CC = gcc
LD = gcc
AR = ar
PYTHON = env python
GIT = env git
ECHO = /bin/echo

INC_PATH = $(HOME_PATH)/include
LIB_PATH = $(HOME_PATH)/lib
BIN = $(HOME_PATH)/bin
API = $(HOME_PATH)/api
INDEX = $(HOME_PATH)/index
SU = $(HOME_PATH)/su
CODEC = $(HOME_PATH)/codec

ifdef USE_GIT_TAG
GIT_SHA = `$(GIT) rev-parse HEAD`
endif

COMPILE_DATE = `date`
COMPILE_HOST = `hostname`

# Get Current # of CPU cores
GET_CORE = `grep -c processor /proc/cpuinfo`

ifeq ($(CPU_CORE),)
__CORES__=$(GET_CORE)
endif

# Debuging the memory and locks
XCFLAGS = -O2 -rdynamic -g -pg -fPIC
DEADCODESTRIP := -fdata-sections -ffunction-sections \
				 -Wl,--gc-sections
O3_OPT = -frename-registers -funroll-loops

CFLAGS += $(XCFLAGS) $(O3_OPT)
CFLAGS += -Wall -DCDATE="\"$(COMPILE_DATE)\"" -DGIT_SHA="\"$(GIT_SHA)\""\
			-DCHOST="\"$(COMPILE_HOST)\"" -I$(INC_PATH) -I$(CODEC) \
			-I$(API) -I$(LIB_PATH) -I$(INDEX) -I$(SU) -D_GNU_SOURCE \
			-DGINGKO_TRACING -DGINGKO_DEBUG_MEMORY -DGINGKO_DEBUG_LOCK_ \
			-D_USE_SPINLOCK_ -DGINGKO_DEBUG_LATENCY_ \
			-DCPU_CORE=$(__CORES__)

ifndef JEMALLOC_HOME
ifeq ($(JEMALLOC),)
else
$(info USE JEMALLOC malloc function @ $(JEMALLOC).)
export JEMALLOC_HOME=$(JEMALLOC)
$(info ========================================================)
endif
endif

ifeq ($(JEMALLOC_HOME),)
else
CFLAGS += -DUSE_JEMALLOC=1 -I$(JEMALLOC_HOME)/include
LFLAGS += -L$(JEMALLOC_HOME)/lib -ljemalloc
endif

LFLAGS += -lrt -ldl -lpthread

# Region for depend files
LIB_AR_SOURCE = lib.c crc32.c md5.c minilzo.c segv.c time.c
SU_AR_SOURCE = su.c dfile.c page.c pagecache.c line.c column.c
INDEX_AR_SOURCE = dfile.c page.c 
CODEC_AR_SOURCE = dict.c delta.c runlen.c
API_AR_SOURCE = api.c

INC_H_SOURCE = atomic.h err.h gingko.h gingko_common.h gingko_const.h \
				gingko_u.h memory.h tracing.h xhash.h xlist.h xlock.h
SU_H_SOURCE = 
INDEX_H_SOURCE = 
LIB_H_SOURCE = lib.h minilzo.h
API_H_SOURCE = 
CODEC_H_SOURCE = 

inc_h_depend_files = $(patsubst %.h, $(INC_PATH)/%.h, $(INC_H_SOURCE)) \
						$(LIB_PATH)/hash.c
su_h_depend_files = $(patsubst %.h, $(SU)/%.h, $(SU_H_SOURCE))
index_h_depend_files = $(patsubst %.h, $(INDEX)/%.h, $(INDEX_H_SOURCE))
lib_h_depend_files = $(patsubst %.h, $(LIB_PATH)/%.h, $(LIB_H_SOURCE))
api_h_depend_files = $(patsubst %.h, $(API)/%.h, $(API_H_SOURCE))
codec_h_depend_files = $(patsubst %.h, $(CODEC)/%.h, $(CODEC_H_SOURCE))

header_depend_files = $(inc_h_depend_files) $(su_h_depend_files) \
						$(lib_h_depend_files) $(index_h_depend_files) \
						$(api_h_depend_files) $(codec_h_depend_files)

su_depend_files = $(patsubst %.c, $(SU)/%.c, $(SU_AR_SOURCE)) \
					$(header_depend_files)
index_depend_files = $(patsubst %.c, $(INDEX)/%.c, $(INDEX_AR_SOURCE)) \
					$(header_depend_files)
lib_depend_files = $(patsubst %.c, $(LIB_PATH)/%.c, $(LIB_AR_SOURCE)) \
					$(header_depend_files)
api_depend_files = $(patsubst %.c, $(API)/%.c, $(API_AR_SOURCE)) \
					$(header_depend_files)
codec_depend_files = $(patsubst %.c, $(CODEC)/%.c, $(CODEC_AR_SOURCE)) \
					$(header_depend_files)

su_o_depend_files = $(patsubst %.c, $(SU)/%.o, $(SU_AR_SOURCE))
index_o_depend_files = $(patsubst %.c, $(INDEX)/%.o, $(INDEX_AR_SOURCE))
lib_o_depend_files = $(patsubst %.c, $(LIB_PATH)/%.o, $(LIB_AR_SOURCE))
api_o_depend_files = $(patsubst %.c, $(API)/%.o, $(API_AR_SOURCE))
codec_o_depend_files = $(patsubst %.c, $(CODEC)/%.o, $(CODEC_AR_SOURCE))

# shared libs
LIB_SO = $(LIB_PATH)/libgingko.so
API_SO = $(LIB_PATH)/libapi.so
SU_SO = $(LIB_PATH)/libsu.so
INDEX_SO = $(LIB_PATH)/libindex.so
CODEC_SO = $(LIB_PATH)/libcodec.so

GINGKO_LIB = $(LIB_PATH)/libgingko.a
API_LIB = $(LIB_PATH)/libapi.a
SU_LIB = $(LIB_PATH)/libsu.a
INDEX_LIB = $(LIB_PATH)/libindex.a
CODEC_LIB = $(LIB_PATH)/libcodec.a

TARGET = $(LIB_PATH)/libgk.so