/*
 *	pg_migrator.c
 *
 *	main source file
 *
 *	Copyright (c) 2010-2020, PostgreSQL Global Development Group
 */
#include "postgres_fe.h"

#include "pg_migrator.h"
#include "catalog/pg_class_d.h"
#include "common/file_perm.h"
#include "common/logging.h"
#include "common/restricted_token.h"
#include "fe_utils/string_utils.h"

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

int
main(int argc, char **argv)
{
	pg_logging_init(argv[0]);
	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_upgrade"));
	parseCommandLine(argc, argv);
	return 0;
}

