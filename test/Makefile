#
# Minimal Migration Manager
# Copyright (C) 2015 Tim Hentenaar.
#
# This code is licenced under the Simplified BSD License.
# See the LICENSE file for details.
#
LIBS=
GCOVR=$(shell which gcovr 2>/dev/null)

# Gather the test sources
SRCS := $(wildcard *.c)

# We want SUSv2 compliant interfaces
CFLAGS = -O2 -g -D_XOPEN_SOURCE=500 -DIN_TESTS
GCOVR_FLAGS = -d -r .. -e '^test/'

# If COVERAGE is defined, generate a coverage report.
ifneq ($(origin COVERAGE), undefined)
ifneq (,$(GCOVR))
  CFLAGS := -O0 -g -coverage $(filter-out -O2,$(CFLAGS))

# Don't remove gcov files when using coveralls
ifneq ($(origin COVERALLS), undefined)
  GCOVR_FLAGS := $(filter-out -d,$(GCOVR_FLAGS))
endif # COVERALLS
endif # GCOVR
endif # COVERAGE

# Detect compiler type and features
include ../compiler.mk

# XXX: In CUnit suite structs, the test and suite names are _NOT_ const.
CFLAGS := $(filter-out -Wwrite-strings,$(CFLAGS))

# XXX: CUnit's ASSERT_PTR cast pointers to void *, discarding const.
CFLAGS := $(filter-out -Wcast-qual, $(CFLAGS))

# We msut have cunit in order to build the tests.
ifeq ($(origin CUNIT_LIBS), undefined)
CUNIT_CFLAGS := $(shell pkg-config --cflags cunit 2>/dev/null)
CUNIT_LIBS   := $(shell pkg-config --libs cunit 2>/dev/null)
endif

ifneq (,$(CUNIT_LIBS))
  CFLAGS += $(CUNIT_CFLAGS)
  LIBS   += $(CUNIT_LIBS)
endif

# XXX: CUnit also doesn't provide a sensible version macro...
ifneq (,$(CUNIT_LIBS))
TEST_CU_VER := $(shell grep CU_VERSION /usr/include/CUnit/CUnit.h | grep 'VERSION' | cut -d ' ' -f 3 | sed -e 's/"//g' -e 's/\.//g' -e 's/-//g')
ifneq (,$(TEST_CU_VER))
  CFLAGS += -DTEST_CU_VER=$(TEST_CU_VER)
else
  CFLAGS += -DTEST_CU_VER=0
endif
endif # CUNIT_LIBS

#
# Targets
#

ifeq (,$(CUNIT_LIBS))
check: ;$(warning CUnit must be installed to build the tests.)
else
check: test_runner
	@./test_runner

test_runner: $(SRCS)
	@echo "  LD $@"
	@$(CC) $(CFLAGS) -o $@ $^ ${LIBS}

ifeq (,$(GCOVR))
coverage: ;$(warning gcovr must be installed for coverage reports.)
else
coverage: check
	@echo
	@$(GCOVR) $(GCOVR_FLAGS)
ifneq ($(origin COVERALLS), undefined)
	@coveralls -r .. --gcov-options '\-lp'
endif
	@$(RM) -f *.gc[nd][oa]
endif # GCOVR
endif # CUNIT_LIBS

all: check

clean:
	@rm -f test_runner

.PHONY: all clean check coverage

