/*
 *	util.c
 *
 *	utility functions
 *
 *	Copyright (c) 2010-2019, PostgreSQL Global Development Group
 */

#include "postgres_fe.h"
#include "common/username.h"

#include "utils.h"

#include <signal.h>

LogOpts		log_opts;

static void pg_log_v(eLogType type, const char *fmt, va_list ap) pg_attribute_printf(2, 0);


/*
 * report_status()
 *
 *	Displays the result of an operation (ok, failed, error message,...)
 */
void
report_status(eLogType type, const char *fmt,...)
{
	va_list		args;
	char		message[MAX_STRING];

	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);

	pg_log(type, "%s\n", message);
}


/* force blank output for progress display */
void
end_progress_output(void)
{
	/*
	 * In case nothing printed; pass a space so gcc doesn't complain about
	 * empty format string.
	 */
	prep_status(" ");
}


/*
 * prep_status
 *
 *	Displays a message that describes an operation we are about to begin.
 *	We pad the message out to MESSAGE_WIDTH characters so that all of the "ok" and
 *	"failed" indicators line up nicely.
 *
 *	A typical sequence would look like this:
 *		prep_status("about to flarb the next %d files", fileCount );
 *
 *		if(( message = flarbFiles(fileCount)) == NULL)
 *		  report_status(PG_REPORT, "ok" );
 *		else
 *		  pg_log(PG_FATAL, "failed - %s\n", message );
 */
void
prep_status(const char *fmt,...)
{
	va_list		args;
	char		message[MAX_STRING];

	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);

	if (strlen(message) > 0 && message[strlen(message) - 1] == '\n')
		pg_log(PG_REPORT, "%s", message);
	else
		/* trim strings that don't end in a newline */
		pg_log(PG_REPORT, "%-*s", MESSAGE_WIDTH, message);
}


static void
pg_log_v(eLogType type, const char *fmt, va_list ap)
{
	char		message[QUERY_ALLOC];

	vsnprintf(message, sizeof(message), _(fmt), ap);

	/* PG_VERBOSE and PG_STATUS are only output in verbose mode */
	/* fopen() on log_opts.internal might have failed, so check it */
	if (((type != PG_VERBOSE && type != PG_STATUS) || log_opts.verbose) &&
		log_opts.internal != NULL)
	{
		if (type == PG_STATUS)
			/* status messages need two leading spaces and a newline */
			fprintf(log_opts.internal, "  %s\n", message);
		else
			fprintf(log_opts.internal, "%s", message);
		fflush(log_opts.internal);
	}

	switch (type)
	{
		case PG_VERBOSE:
			if (log_opts.verbose)
				printf("%s", message);
			break;

		case PG_STATUS:
			/* for output to a display, do leading truncation and append \r */
			if (isatty(fileno(stdout)))
				/* -2 because we use a 2-space indent */
				printf("  %s%-*.*s\r",
				/* prefix with "..." if we do leading truncation */
					   strlen(message) <= MESSAGE_WIDTH - 2 ? "" : "...",
					   MESSAGE_WIDTH - 2, MESSAGE_WIDTH - 2,
				/* optional leading truncation */
					   strlen(message) <= MESSAGE_WIDTH - 2 ? message :
					   message + strlen(message) - MESSAGE_WIDTH + 3 + 2);
			else
				printf("  %s\n", message);
			break;

		case PG_REPORT:
		case PG_WARNING:
			printf("%s", message);
			break;

		case PG_FATAL:
			printf("\n%s", message);
			printf(_("Failure, exiting\n"));
			exit(1);
			break;

		default:
			break;
	}
	fflush(stdout);
}


void
pg_log(eLogType type, const char *fmt,...)
{
	va_list		args;

	va_start(args, fmt);
	pg_log_v(type, fmt, args);
	va_end(args);
}


void
pg_fatal(const char *fmt,...)
{
	va_list		args;

	va_start(args, fmt);
	pg_log_v(PG_FATAL, fmt, args);
	va_end(args);
	printf(_("Failure, exiting\n"));
	exit(1);
}


void
check_ok(void)
{
	/* all seems well */
	report_status(PG_REPORT, "ok");
	fflush(stdout);
}


/*
 * quote_identifier()
 *		Properly double-quote a SQL identifier.
 *
 * The result should be pg_free'd, but most callers don't bother because
 * memory leakage is not a big deal in this program.
 */
char *
quote_identifier(const char *s)
{
	char	   *result = pg_malloc(strlen(s) * 2 + 3);
	char	   *r = result;

	*r++ = '"';
	while (*s)
	{
		if (*s == '"')
			*r++ = *s;
		*r++ = *s;
		s++;
	}
	*r++ = '"';
	*r++ = '\0';

	return result;
}


/*
 * get_user_info()
 */
int
get_user_info(char **user_name_p)
{
	int			user_id;
	const char *user_name;
	char	   *errstr;

#ifndef WIN32
	user_id = geteuid();
#else
	user_id = 1;
#endif

	user_name = get_user_name(&errstr);
	if (!user_name)
		pg_fatal("%s\n", errstr);

	/* make a copy */
	*user_name_p = pg_strdup(user_name);

	return user_id;
}


/*
 *	str2uint()
 *
 *	convert string to oid
 */
unsigned int
str2uint(const char *str)
{
	return strtoul(str, NULL, 10);
}


/*
 *	pg_putenv()
 *
 *	This is like putenv(), but takes two arguments.
 *	It also does unsetenv() if val is NULL.
 */
void
pg_putenv(const char *var, const char *val)
{
	if (val)
	{
#ifndef WIN32
		char	   *envstr;

		envstr = psprintf("%s=%s", var, val);
		putenv(envstr);

		/*
		 * Do not free envstr because it becomes part of the environment on
		 * some operating systems.  See port/unsetenv.c::unsetenv.
		 */
#else
		SetEnvironmentVariableA(var, val);
#endif
	}
	else
	{
#ifndef WIN32
		unsetenv(var);
#else
		SetEnvironmentVariableA(var, "");
#endif
	}
}

/*
 * Add two Size values, checking for overflow
 */
Size
add_size(Size s1, Size s2)
{
	Size		result;

	result = s1 + s2;
	/* We are assuming Size is an unsigned type here... */
	if (result < s1 || result < s2)
		pg_log(PG_FATAL, "requested shared memory size overflows size_t");
	return result;
}

/*
 * Multiply two Size values, checking for overflow
 */
Size
mul_size(Size s1, Size s2)
{
	Size		result;

	if (s1 == 0 || s2 == 0)
		return 0;
	result = s1 * s2;
	/* We are assuming Size is an unsigned type here... */
	if (result / s2 != s1)
		pg_log(PG_FATAL, "requested shared memory size overflows size_t");
	return result;
}
/*
 * bms_hash_value - compute a hash key for a Bitmapset
 *
 * Note: we must ensure that any two bitmapsets that are bms_equal() will
 * hash to the same value; in practice this means that trailing all-zero
 * words must not affect the result.  Hence we strip those before applying
 * hash_any().
 */
uint32
bms_hash_value(const Bitmapset *a)
{
	int			lastword;

	if (a == NULL)
		return 0;				/* All empty sets hash to 0 */
	for (lastword = a->nwords; --lastword >= 0;)
	{
		if (a->words[lastword] != 0)
			break;
	}
	if (lastword < 0)
		return 0;				/* All empty sets hash to 0 */
	return DatumGetUInt32(hash_any((const unsigned char *) a->words,
								   (lastword + 1) * sizeof(bitmapword)));
}
