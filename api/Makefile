##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2013-09-18 15:01:39 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

include ../Makefile.inc

all : api_lib

%.o : %.c $(api_h_depend_files)
	@$(ECHO) -e " " CC"\t" $@
	@$(CC) $(CFLAGS) -c $(patsubst %.c, $(API)/%.c, $<)

api_lib : $(API_AR_SOURCE:.c=.o)
	@$(ECHO) -e " " AR"\t" $@
	@$(AR) rcs libapi.a $(^:.c=.o)
	@$(ECHO) -e " " SL"\t" $(API_SO)
	@$(CC) -shared -Wl,-soname,libapi.so -o $(LIB_PATH)/libapi.so $(^:.c=.o) -lc -lrt -lpthread

clean : 
	-@rm -rf $(API_AR_SOURCE:.c=.o)
	-@rm -rf libapi.a $(LIB_PATH)/libapi.so