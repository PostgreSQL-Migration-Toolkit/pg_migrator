# contrib/libhash/Makefile

MODULE_big = libhash
OBJS = utils.o dynahash.o hashfn.o pg_crc.o $(WIN32RES)
PGFILEDESC = "libhash"

PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK_INTERNAL = $(libpq)

EXTENSION = libhash

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
