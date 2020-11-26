/*
 *	option.c
 *
 *	options functions
 *
 *	Copyright (c) 2010-2020, PostgreSQL Global Development Group
 *	src/bin/pg_migrator/option.c
 */

#include "postgres_fe.h"
#include <time.h>
#ifdef WIN32
#include <io.h>
#endif

#include "getopt_long.h"
#include "utils/pidfile.h"

#include "pg_migrator.h"


Opts opts;

#define PG_MIGRATOR_VERSION "0.1.0"

static void usage(void);


/*
 * parseCommandLine()
 *
 *	Parses the command line (argc, argv[]) and loads structures
 */
void
parseCommandLine(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"verbose", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};
	int			option;			/* Command line option */
	int			optindex = 0;	/* used by getopt_long */

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage();
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("pg_migrator (PostgreSQL) " PG_MIGRATOR_VERSION);
			exit(0);
		}
	}

	while ((option = getopt_long(argc, argv, "v", long_options, &optindex)) != -1)
	{
		switch (option)
		{
			case 'v':
				opts.verbose = 1;
				break;
			default:
				fprintf(stderr, _("Try \"%s --help\" for more information.\n"),
						"pg_migrator");
				exit(1);
		}
	}
}


static void
usage(void)
{
	printf(_("pg_migrator migrats to PostgreSQL cluster from different DBMS(Oracle/MySQL).\n\n"));
	printf(_("Usage:\n"));
	printf(_("  pg_migrator [OPTION]...\n\n"));
	printf(_("Options:\n"));
	printf(_("  -v, --verbose                 enable verbose internal logging\n"));
	printf(_("  -V, --version                 display version information, then exit\n"));
	printf(_("  -?, --help                    show this help, then exit\n"));
	printf(_("\nReport bugs to <pgsql-bugs@lists.postgresql.org>.\n"));
}
