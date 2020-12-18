# src/bin/pg_migrator/Makefile

PGFILEDESC = "pg_migrator - a database migration utility"

PGAPPICON = win32

PROGRAM = pg_migrator
OBJS 	 = oci_db.o guc.o util.o option.o pg_migrator.o $(WIN32RES)

TAP_TESTS = 1

PG_CPPFLAGS = -I$(libpq_srcdir) $(shell pkg-config --cflags glib-2.0) -I/opt/oracle/product/18c/dbhomeXE/sdk/include/ -I/opt/oracle/product/18c/dbhomeXE/rdbms/public
PG_LIBS_INTERNAL = $(libpq_pgport) $(shell pkg-config --libs glib-2.0) -L/opt/oracle/product/18c/dbhomeXE/lib -locci -lclntsh

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

