##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2013-09-18 15:17:11 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

include ../Makefile.inc

all : index_lib

%.o : %.c $(index_h_depend_files)
	@$(ECHO) -e " " CC"\t" $@
	@$(CC) $(CFLAGS) -c $(patsubst %.c, $(INDEX)/%.c, $<)

index_lib : $(INDEX_AR_SOURCE:.c=.o)
	@$(ECHO) -e " " AR"\t" $@
	@$(AR) rcs libindex.a $(^:.c=.o)
	@$(ECHO) -e " " SL"\t" $(INDEX_SO)
	@$(CC) -shared -Wl,-soname,libindex.so -o $(LIB_PATH)/libindex.so $(^:.c=.o) -lc -lrt -lpthread

clean : 
	-@rm -rf $(INDEX_AR_SOURCE:.c=.o)
	-@rm -rf libindex.a $(LIB_PATH)/libindex.so