/*
 *	pg_migrator.h
 *
 *	Copyright (c) 2010-2019, PostgreSQL Global Development Group
 */

#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/time.h>

#define MAX_STRING			1024
#define LINE_ALLOC			4096
#define QUERY_ALLOC			8192
#define MESSAGE_WIDTH		60


/* option.c */
typedef struct Opts
{
	int verbose;
} Opts;

void parseCommandLine(int argc, char *argv[]);


/* guc.c */
int read_config_file(void);
char* get_source_guc(char *guc);
char* get_destination_guc(char *guc);
char* get_source_type(void);
char* get_source_dbusername(void);
char* get_source_dbpassword(void);
char* get_source_dbhost(void);

/* oci_deparse.c */
char* deparse_system_users(void);
char* deparse_table_query(void);

/* util.c */

/*
 * Enumeration to denote pg_log modes
 */
typedef enum
{
	PG_VERBOSE,
	PG_STATUS,
	PG_REPORT,
	PG_WARNING,
	PG_FATAL
} eLogType;

/*
 *	LogOpts
*/
typedef struct
{
	FILE	   *internal;		/* internal log FILE */
	bool		verbose;		/* true -> be verbose in messages */
	bool		retain;			/* retain log files on success */
} LogOpts;

char	   *quote_identifier(const char *s);
int			get_user_info(char **user_name_p);
void		check_ok(void);
void		report_status(eLogType type, const char *fmt,...) pg_attribute_printf(2, 3);
void		pg_log(eLogType type, const char *fmt,...) pg_attribute_printf(2, 3);
void		pg_fatal(const char *fmt,...) pg_attribute_printf(1, 2) pg_attribute_noreturn();
void		end_progress_output(void);
void		prep_status(const char *fmt,...) pg_attribute_printf(1, 2);
void		check_ok(void);
unsigned int str2uint(const char *str);
void		pg_putenv(const char *var, const char *val);

