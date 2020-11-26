# src/bin/pg_migrator/Makefile

PGFILEDESC = "pg_migrator - a database migration utility"

PGAPPICON = win32

PROGRAM = pg_migrator
OBJS 	 = util.o option.o pg_migrator.o $(WIN32RES)

TAP_TESTS = 1

PG_CPPFLAGS = -I$(libpq_srcdir)
PG_LIBS_INTERNAL = $(libpq_pgport)

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = pg_migrator
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

