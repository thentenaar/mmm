#
# Minimal Migration Manager
# Copyright (C) 2015 Tim Hentenaar.
#
# This code is licenced under the Simplified BSD License.
# See the LICENSE file for details.
#
LIBS=
INSTALL := $(shell which install 2>/dev/null)
PREFIX ?= /usr

# We want SUSv2 compliant interfaces
CFLAGS=-O2 -D_XOPEN_SOURCE=500

# Gather the main sources
SRCS := $(wildcard src/*.c) src/source/file.c
HS   := $(wildcard src/*.h src/*/*.h)

# Detect the compiler type and features
include compiler.mk

ifeq ($(origin INDENT), undefined)
INDENT := $(shell which indent 2>/dev/null)
endif

#
# Handle features
#

# Check for libgit2
ifeq ($(origin GIT2_LIBS), undefined)
GIT2_CFLAGS:=$(shell pkg-config --cflags libgit2 2>/dev/null)
GIT2_LIBS:=$(shell pkg-config --libs libgit2 2>/dev/null)
endif

ifneq (,$(GIT2_LIBS))
  CFLAGS += -DHAVE_GIT $(GIT2_CFLAGS)
  LIBS   += $(GIT2_LIBS)
  SRCS   += src/source/git.c
endif

# Check for sqlite3
ifeq ($(origin SQLITE3_LIBS), undefined)
SQLITE3_CFLAGS := $(shell pkg-config --cflags sqlite3 2>/dev/null)
SQLITE3_LIBS   := $(shell pkg-config --libs sqlite3 2>/dev/null)
endif

ifneq (,$(SQLITE3_LIBS))
  CFLAGS += -DHAVE_SQLITE3 $(SQLITE3_CFLAGS)
  LIBS   += $(SQLITE3_LIBS)
  SRCS   += src/db/sqlite3.c
endif

# Check for postgresql
ifeq ($(origin PGSQL_LIBDIR), undefined)
PGSQL_LIBDIR := $(shell pg_config --libdir 2>/dev/null)
PGSQL_INCDIR := $(shell pg_config --includedir 2>/dev/null)
endif

ifneq (,$(PGSQL_LIBDIR))
  CFLAGS += -DHAVE_PGSQL -I$(PGSQL_INCDIR)
  LIBS   += -L$(PGSQL_LIBDIR) -lpq
  SRCS   += src/db/pgsql.c
endif

# Check for libmysqlclient
ifeq ($(origin MYSQL_LIBS), undefined)
MYSQL_CFLAGS := $(shell mysql_config --cflags 2>/dev/null)
MYSQL_LIBS   := $(shell mysql_config --libs 2>/dev/null)
endif

ifneq (,$(MYSQL_LIBS))
  CFLAGS += -DHAVE_MYSQL $(MYSQL_CFLAGS)
  LIBS   += $(MYSQL_LIBS)
  SRCS   += src/db/mysql.c
endif

# We should have at least one database driver...
ifeq (,$(SQLITE3_LIBS))
ifeq (,$(PGSQL_LIBDIR))
ifeq (,$(MYSQL_LIBS))
  $(error No database drivers would be built.\
          Please install libpq, libmysqlclient or sqlite3)
endif
endif
endif

# Check for use of OpenSSL
ifneq (,$(findstring -lssl,$(LIBS)))
  CFLAGS += -DUSE_OPENSSL
endif

# .c to .o
OBJS  = ${SRCS:.c=.o}

#
# Targets
#
mmm: $(OBJS)
	@echo "  LD $@"
	@$(CC) -o $@ $^ $(LIBS)

all: mmm

install: mmm
	@echo " INSTALL mmm -> $(PREFIX)/bin/mmm"
	@mkdir -p $(PREFIX)/bin
	@$(INSTALL) -s -m0755 mmm $(PREFIX)/bin/mmm
	@echo " INSTALL mmm.1 -> $(PREFIX)/share/man/man1/mmm.1"
	@mkdir -p $(PREFIX)/share/man/man1
	@$(INSTALL) -m0644 mmm.1 $(PREFIX)/share/man/man1/mmm.1

uninstall:
	@echo " UNINSTALL mmm"
	@$(RM) -f $(PREFIX)/bin/mmm
	@echo " UNINSTALL mmm.1"
	@$(RM) -f $(PREFIX)/share/man/man1/mmm.1*

clean:
	@$(MAKE) -C test clean
	@$(RM) -f $(OBJS) mmm

check:
	@$(MAKE) -C test check

coverage:
	@COVERAGE=1 $(MAKE) -C test coverage

indent:
ifneq (,$(INDENT))
	@echo "  INDENT src/*/*.[ch]"
	@VERSION_CONTROL=none $(INDENT) $(SRCS) $(HS)
else
	@echo "'indent' not found."
endif

.c.o:
	@echo "  CC $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

.SUFFIXES: .c .o
.PHONY: all install uninstall clean check coverage indent

