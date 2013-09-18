##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2013-09-18 15:00:41 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

include ../Makefile.inc

all : codec_lib

%.o : %.c $(codec_h_depend_files)
	@$(ECHO) -e " " CC"\t" $@
	@$(CC) $(CFLAGS) -c $(patsubst %.c, $(CODEC)/%.c, $<)

codec_lib : $(CODEC_AR_SOURCE:.c=.o)
	@$(ECHO) -e " " AR"\t" $@
	@$(AR) rcs libcodec.a $(^:.c=.o)
	@$(ECHO) -e " " SL"\t" $(CODEC_SO)
	@$(CC) -shared -Wl,-soname,libcodec.so -o $(LIB_PATH)/libcodec.so $(^:.c=.o) -lc -lrt -lpthread

clean : 
	-@rm -rf $(CODEC_AR_SOURCE:.c=.o)
	-@rm -rf libcodec.a $(LIB_PATH)/libcodec.so