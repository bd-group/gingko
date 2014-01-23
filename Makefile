##
# Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
#                           <macan@ncic.ac.cn>
#
# Time-stamp: <2014-01-23 16:29:25 macan>
#
# This is the makefile for GINGKO project.
#
# Armed by EMACS.

HOME_PATH = $(shell pwd)

include Makefile.inc

all : $(TARGET) test

$(GINGKO_LIB) : $(lib_depend_files)
	@$(ECHO) -e " " CD"\t" $(LIB_PATH)
	@$(ECHO) -e " " MK"\t" $@
	@$(MAKE) --no-print-directory -C $(LIB_PATH) -e "HOME_PATH=$(HOME_PATH)"

$(SU_LIB) : $(su_depend_files)
	@$(ECHO) -e " " CD"\t" $(SU)
	@$(ECHO) -e " " MK"\t" $@
	@$(MAKE) --no-print-directory -C $(SU) -e "HOME_PATH=$(HOME_PATH)"

$(INDEX_LIB) : $(index_depend_files)
	@$(ECHO) -e " " CD"\t" $(INDEX)
	@$(ECHO) -e " " MK"\t" $@
	@$(MAKE) --no-print-directory -C $(INDEX) -e "HOME_PATH=$(HOME_PATH)"

$(CODEC_LIB) : $(codec_depend_files)
	@$(ECHO) -e " " CD"\t" $(CODEC)
	@$(ECHO) -e " " MK"\t" $@
	@$(MAKE) --no-print-directory -C $(CODEC) -e "HOME_PATH=$(HOME_PATH)"

$(API_LIB) : $(api_depend_files)
	@$(ECHO) -e " " CD"\t" $(API)
	@$(ECHO) -e " " MK"\t" $@
	@$(MAKE) --no-print-directory -C $(API) -e "HOME_PATH=$(HOME_PATH)"

$(TARGET) : lib
	@$(CC) -shared -Wl,-soname,libgk.so -o $(LIB_PATH)/libgk.so $(su_o_depend_files) \
		$(index_o_depend_files) $(lib_o_depend_files) $(api_o_depend_files) \
		$(codec_o_depend_files) $(LFLAGS)
	@$(ECHO) -e " " Target is ready @ $(TARGET)

clean :
	@$(MAKE) --no-print-directory -C $(LIB_PATH) -e "HOME_PATH=$(HOME_PATH)" clean
	@$(MAKE) --no-print-directory -C $(SU) -e "HOME_PATH=$(HOME_PATH)" clean
	@$(MAKE) --no-print-directory -C $(INDEX) -e "HOME_PATH=$(HOME_PATH)" clean
	@$(MAKE) --no-print-directory -C $(API) -e "HOME_PATH=$(HOME_PATH)" clean
	@$(MAKE) --no-print-directory -C $(CODEC) -e "HOME_PATH=$(HOME_PATH)" clean
	@$(MAKE) --no-print-directory -C $(TEST) -e "HOME_PATH=$(HOME_PATH)" clean

help :
	@$(ECHO) "Environment Variables:"
	@$(ECHO) ""
	@$(ECHO) "1. JEMALLOC          Must defined w/ jemalloc install path prefix;"
	@$(ECHO) "                     otherwise, we can find the jemalloc lib path."
	@$(ECHO) ""

$(JEMALLOC_RULE) :
	@$(ECHO) $(JEMALLOC_RULE)
	@$(ECHO) -e " " CD"\t" $(DEPS)
	@$(ECHO) -e " " MK"\t" $@
	@$(MAKE) --no-print-directory -C $(DEPS) -e "HOME_PATH=$(HOME_PATH)" $(JEMALLOC_TARGET)

lib : $(JEMALLOC_RULE) $(GINGKO_LIB) $(SU_LIB) $(INDEX_LIB) $(CODEC_LIB) $(API_LIB)
	@$(ECHO) -e " " Lib is ready.

test : $(TARGET)
	@$(ECHO) -e " " Build Unit Testcases ...
	@$(ECHO) -e " " CD"\t" $(TEST)
	@$(MAKE) --no-print-directory -C $(TEST) -e "HOME_PATH=$(HOME_PATH)"
