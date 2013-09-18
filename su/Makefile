##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2013-09-18 14:59:32 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

include ../Makefile.inc

all : su_lib

%.o : %.c $(su_h_depend_files)
	@$(ECHO) -e " " CC"\t" $@
	@$(CC) $(CFLAGS) -c $(patsubst %.c, $(SU)/%.c, $<)

su_lib : $(SU_AR_SOURCE:.c=.o)
	@$(ECHO) -e " " AR"\t" $@
	@$(AR) rcs libsu.a $(^:.c=.o)
	@$(ECHO) -e " " SL"\t" $(SU_SO)
	@$(CC) -shared -Wl,-soname,libsu.so -o $(LIB_PATH)/libsu.so $(^:.c=.o) -lc -lrt -lpthread

clean : 
	-@rm -rf $(SU_AR_SOURCE:.c=.o)
	-@rm -rf libsu.a $(LIB_PATH)/libsu.so