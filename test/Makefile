##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2013-11-12 16:34:00 macan>
#
# This is the makefile for GINGKO project.
#
# Armed with EMACS.

include ../Makefile.inc

%.u : %.c $(TEST_BUILD_SOURCE)
	@$(ECHO) -e " " CC"\t" $@
	@$(CC) $(CFLAGS) $(patsubst %.c, $(TEST)/%.c, $<) -o $(patsubst %.c, $(TEST)/%.u, $<) \
		-L$(LIB_PATH) -lgk -lgingko -lc -lrt -lpthread

all : $(TEST_BUILD_SOURCE:.c=.u)
	@$(ECHO) -e " " Build Unit Testcase Done.

clean : 
	-@rm -rf $(TEST_BUILD_SOURCE:.c=.u)
