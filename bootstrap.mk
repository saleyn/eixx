# vim:ts=4:sw=4:et
#-------------------------------------------------------------------------------
# Bootstrapping cmake
#-------------------------------------------------------------------------------
# Copyright (c) 2015 Serge Aleynikov
# Date: 2014-08-12
#-------------------------------------------------------------------------------

PROJECT  := $(shell sed -n '/^project/{s/^project. *\([a-zA-Z0-9]\+\).*/\1/p; q}'\
                    CMakeLists.txt)
VERSION  := $(shell sed -n '/^project/{s/^.\+VERSION \+//; s/[^\.0-9]\+//; p; q}'\
                    CMakeLists.txt)

HOSTNAME := $(shell hostname)

# Options file is either: .cmake-args.$(HOSTNAME) or .cmake-args
OPT_FILE     := .cmake-args.$(HOSTNAME)
ifeq "$(wildcard $(OPT_FILE))" ""
    OPT_FILE := .cmake-args
    ifeq "$(wildcard $(OPT_FILE))" ""
        OPT_FILE := "/dev/null"
    endif
endif

#-------------------------------------------------------------------------------
# Default target
#-------------------------------------------------------------------------------
all:
	@echo
	@echo "Run: make bootstrap [toolchain=gcc|clang|intel] [verbose=true] \\"
	@echo "                    [generator=ninja|make] [build=Debug|Release]"
	@echo
	@echo "To customize cmake variables, create a file with VAR=VALUE pairs:"
	@echo "  '.cmake-args.$(HOSTNAME)' or '.cmake-args'"
	@echo ""
	@echo "There are three sets of variables present there:"
	@echo "  1. DIR:BUILD=...   - Build directory"
	@echo "     DIR:INSTALL=... - Install directory"
	@echo
	@echo "     They may contain macros:"
	@echo "         @PROJECT@   - name of current project (from CMakeList.txt)"
	@echo "         @VERSION@   - project version number  (from CMakeList.txt)"
	@echo "         @BUILD@     - build type (from command line)"
	@echo "         \$${...}      - environment variable"
	@echo
	@echo "  2. ENV:VAR=...     - Environment var set before running cmake"
	@echo
	@echo "  3. VAR=...         - Variable passed to cmake with -D prefix"
	@echo
	@echo "  Lines beginning with '#' are considered to be comments"
	@echo

toolchain   ?= gcc
build       ?= Debug
# Convert build to lower case:
BUILD       := $(shell echo $(build) | tr 'A-Z' 'a-z')

ifeq (,$(findstring $(BUILD),debug release relwithdefinfo minsizerel))
    $(error Invalid build type: $(build))
endif

# Function that replaces variables in a given entry in a file 
# E.g.: $(call substitute,ENTRY,FILENAME)
substitute   = $(shell sed -n '/^$(1)=/{s!$(1)=!!; s!/\+$$!!;     \
                                       s!@PROJECT@!$(PROJECT)!gI; \
                                       s!@VERSION@!$(VERSION)!gI; \
                                       s!@BUILD@!$(BUILD)!gI;     \
                                       p; q}' $(2) 2>/dev/null)
BLD_DIR     := $(call substitute,DIR:BUILD,$(OPT_FILE))
ROOT_DIR    := $(dir $(abspath include))
DEF_BLD_DIR := $(ROOT_DIR:%/=%)/build
DIR         := $(if $(BLD_DIR),$(BLD_DIR),$(DEF_BLD_DIR))
PREFIX      := $(call substitute,DIR:INSTALL,$(OPT_FILE))
prefix      ?= $(if $(PREFIX),$(PREFIX),/usr/local)
generator   ?= make

#-------------------------------------------------------------------------------
# info target
#-------------------------------------------------------------------------------
info:
	@echo "PROJECT:   $(PROJECT)"
	@echo "HOSTNAME:  $(HOSTNAME)"
	@echo "VERSION:   $(VERSION)"
	@echo "OPT_FILE:  $(OPT_FILE)"
	@echo "BLD_DIR:   $(BLD_DIR)"
	@echo "DIR:       $(DIR)"
	@echo "build:     $(BUILD)"
	@echo "prefix:    $(prefix)"
	@echo "generator: $(generator)"

ver:
	@echo $(VERSION)

variables   := $(filter-out toolchain=% generator=% build=% verbose=% prefix=%,$(MAKEOVERRIDES))
makevars    := $(variables:%=-D%)

envvars     += $(shell sed -n '/^ENV:/{s/^ENV://;p}' $(OPT_FILE) 2>/dev/null)
makevars    += $(patsubst %,-D%,$(shell sed -n '/^...:/!{s/ *\#.*$$//; /^$$/!p}' \
                                            $(OPT_FILE) 2>/dev/null))

makecmd      = $(envvars) cmake -H. -B$(DIR) \
               $(if $(findstring $(generator),ninja),-GNinja,-G'Unix Makefiles') \
               $(if $(findstring $(verbose),true on 1),-DCMAKE_VERBOSE_MAKEFILE=true) \
               -DTOOLCHAIN=$(toolchain) \
               -DCMAKE_USER_MAKE_RULES_OVERRIDE=$(ROOT_DIR:%/=%)/build-aux/CMakeInit.txt \
               -DCMAKE_INSTALL_PREFIX=$(prefix) \
               -DCMAKE_BUILD_TYPE=$(build) $(makevars)

#-------------------------------------------------------------------------------
# bootstrap target
#-------------------------------------------------------------------------------
bootstrap: | $(DIR)
    ifeq "$(generator)" ""
		@echo -e "\n\e[1;31mBuild tool not specified!\e[0m\n" && false
    else ifeq "$(shell which $(generator) 2>/dev/null)" ""
		@echo -e "\n\e[1;31mBuild tool $(generator) not found!\e[0m\n" && false
    endif
	@echo -e "Options file.....: $(OPT_FILE)"
	@echo -e "Build directory..: \e[0;36m$(DIR)\e[0m"
	@echo -e "Install directory: \e[0;36m$(prefix)\e[0m"
	@echo -e "Build type.......: \e[1;32m$(BUILD)\e[0m"
	@echo -e "Command-line vars: $(variables)"
	@echo -e "\n-- \e[1;37mUsing $(generator) generator\e[0m\n"
	@mkdir -p .build
	@rm -f inst
	@[ -L build ] && rm -f build || true
	@echo $(call makecmd) > $(DIR)/.cmake
	$(call makecmd) 2>&1 | tee $(DIR)/.cmake.bootstrap.log
	@[ ! -d build ] && ln -s $(DIR) build || true
	@ln -s $(prefix) inst
	@echo "make bootstrap $(MAKEOVERRIDES)"            >  $(DIR)/.bootstrap
	@cp $(DIR)/.bootstrap .build/
	@echo "export PROJECT   := $(PROJECT)"             >  $(DIR)/cache.mk
	@echo "export VERSION   := $(VERSION)"             >> $(DIR)/cache.mk
	@echo "export OPT_FILE  := $(abspath $(OPT_FILE))" >> $(DIR)/cache.mk
	@echo "export generator := $(generator)"           >> $(DIR)/cache.mk
	@echo "export build     := $(BUILD)"               >> $(DIR)/cache.mk
	@echo "export DIR       := $(DIR)"                 >> $(DIR)/cache.mk
	@echo "export prefix    := $(prefix)"              >> $(DIR)/cache.mk

$(DIR):
	@mkdir -p $@

.PHONY: all bootstrap info
