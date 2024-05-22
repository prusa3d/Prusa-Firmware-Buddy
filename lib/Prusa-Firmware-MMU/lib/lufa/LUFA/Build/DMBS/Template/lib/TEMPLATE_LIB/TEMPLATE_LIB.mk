#
#            DMBS Build System
#     Released into the public domain.
#
#  dean [at] fourwalledcubicle [dot] com
#        www.fourwalledcubicle.com
#

# Include Guard
ifeq ($(filter TEMPLATE_LIB, $(DMBS_BUILD_MODULES)),)

# Sanity check user supplied DMBS path
ifndef DMBS_PATH
$(error Makefile DMBS_PATH option cannot be blank)
endif

# Location of the current module
TEMPLATE_LIB_MODULE_PATH := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

# Import the CORE module of DMBS
include $(DMBS_PATH)/core.mk

# This module needs to be included before gcc.mk
ifneq ($(findstring GCC, $(DMBS_BUILD_MODULES)),)
$(error Include this module before gcc.mk)
endif

# Help settings
DMBS_BUILD_MODULES         += TEMPLATE_LIB
DMBS_BUILD_TARGETS         +=
DMBS_BUILD_MANDATORY_VARS  += DMBS_PATH
DMBS_BUILD_OPTIONAL_VARS   +=
DMBS_BUILD_PROVIDED_VARS   += TEMPLATE_LIB_SRC
DMBS_BUILD_PROVIDED_MACROS +=

# Sanity check user supplied values
$(foreach MANDATORY_VAR, $(DMBS_BUILD_MANDATORY_VARS), $(call ERROR_IF_UNSET, $(MANDATORY_VAR)))

# TEMPLATE_LIB Library
TEMPLATE_LIB_SRC := $(TEMPLATE_LIB_MODULE_PATH)/src/template_lib.c

# Compiler flags and sources
SRC			       += $(TEMPLATE_LIB_SRC)
CC_FLAGS           += -DDMBS_MODULE_TEMPLATE_LIB
CC_FLAGS 	       += -I$(TEMPLATE_LIB_MODULE_PATH)/include/

# Phony build targets for this module
.PHONY: $(DMBS_BUILD_TARGETS)

endif
