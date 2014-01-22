##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2014-01-21 10:22:12 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

include ../Makefile.inc

all : gingko_lib

%.o : %.c 
	@$(ECHO) -e " " CC"\t" $@
	@$(CC) $(CFLAGS) -c $(patsubst %.c, $(LIB_PATH)/%.c, $<)

gingko_lib : $(LIB_AR_SOURCE:.c=.o)
	@$(ECHO) -e " " AR"\t" $@
	@$(AR) rcs libgingko.a $(^:.c=.o)
	@$(ECHO) -e " " SL"\t" $(LIB_SO)
	@$(CC) -shared -Wl,-soname,libgingko.so -o $(LIB_PATH)/libgingko.so $(^:.c=.o) -lc -lrt -lpthread

clean : 
	-@rm -rf $(LIB_AR_SOURCE:.c=.o)
	-@rm -rf *.a *.so *.so.profile