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

#include "db.h"

#define MAX_STRING 1024
int
main(int argc, char **argv)
{
	char v1[MAX_STRING];
	char v2[MAX_STRING];
	char v3[MAX_STRING];
	char v4[MAX_STRING];
	int r;
	
	/* initialize of the logging system */
	pg_logging_init(argv[0]);

	set_pglocale_pgservice(argv[0], PG_TEXTDOMAIN("pg_migrator"));

	/* read the command line options */
	parseCommandLine(argc, argv);

	/* read the configuration file */
	prep_status("reading configuration file");
	read_config_file();
	check_ok();


	prep_status("establishing remote session with source (%s)", get_source_dbhost());
	r = dbinit();
	if (r < 0)
		return 0;

	r = dbconnect(get_source_dbusername(), get_source_dbpassword());
	if (r < 0)
		return 0;
	check_ok();
	
	prep_status("extracting data");
	r = dbbegin();
	if (r < 0)
		return 0;
	
	r = dbprepare(deparse_table_query());
	if (r < 0)
		return 0;
	
	r = dbbind_string(&v1, 1, MAX_STRING);
	if (r < 0)
		return 0;
	
	r = dbbind_string(&v2, 2, MAX_STRING);
	if (r < 0)
		return 0;
	
	r = dbbind_string(&v3, 3, MAX_STRING);
	if (r < 0)
		return 0;
	
	r = dbbind_string(&v4, 4, MAX_STRING);
	if (r < 0)
		return 0;
	
	r = dbexecute();
	if (r < 0)
	{
		pg_log(PG_FATAL,"\npg_stat_monitor: %s\n\t%s", "failed to execute query", get_db_log());
		return 0;
	}
	for(;;)
	{
		printf("\n%s\t%s\t%s\t%s\n", v1, v2, v3, v4);
		r = dbfetch();
		if (r <= 0)
			break;
	}
	check_ok();
	prep_status("disconnecting from remote source session (%s)", get_source_dbhost());
	dbcleanup();
	check_ok();
	return 0;
}

char*
deparse_system_users(void)
{
	char *query = "SELECT username from dba_users where username not in ('ANONYMOUS'\
	,'APEX_040200'\
	,'APEX_PUBLIC_USER'\
	,'APPQOSSYS'\
	,'AUDSYS'\
	,'BI'\
	,'CTXSYS'\
	,'DBSNMP'\
	,'DIP'\
	,'DVF'\
	,'DVSYS'\
	,'EXFSYS'\
	,'FLOWS_FILES'\
	,'GSMADMIN_INTERNAL'\
	,'GSMCATUSER'\
	,'GSMUSER'\
	,'HR'\
	,'IX'\
	,'LBACSYS'\
	,'MDDATA'\
	,'MDSYS'\
	,'OE'\
	,'ORACLE_OCM'\
	,'ORDDATA'\
	,'ORDPLUGINS'\
	,'ORDSYS'\
	,'OUTLN'\
	,'PM'\
	,'SCOTT'\
	,'SH'\
	,'SI_INFORMTN_SCHEMA'\
	,'SPATIAL_CSW_ADMIN_USR'\
	,'SPATIAL_WFS_ADMIN_USR'\
	,'SYS'\
	,'SYSBACKUP'\
	,'SYSDG'\
	,'SYSKM'\
	,'SYSTEM'\
	,'WMSYS'\
	,'XDB'\
	,'SYSMAN'\
	,'RMAN'\
	,'RMAN_BACKUP'\
	,'OWBSYS'\
	,'OWBSYS_AUDIT'\
	,'APEX_030200'\
	,'MGMT_VIEW'\
	,'OJVMSYS')";

	return query;
}

char table_info_query[1024]; 

char*
deparse_table_query(void)
{
	sprintf(table_info_query, "%s (%s) %s", 
						"SELECT A.TABLE_NAME,				\
							A.COMMENTS,						\
							A.TABLE_TYPE,					\
							A.OWNER							\
							FROM							\
					 		ALL_TAB_COMMENTS A,				\
					 		ALL_OBJECTS O					\
						WHERE 								\
							A.OWNER = O.OWNER				\
						AND A.TABLE_NAME = O.OBJECT_NAME	\
						AND O.OBJECT_TYPE = 'TABLE'			\
						AND A.OWNER IN ", deparse_system_users()," AND (A.OWNER, A.TABLE_NAME) \
							NOT IN (SELECT OWNER, MVIEW_NAME FROM ALL_MVIEWS UNION ALL SELECT LOG_OWNER, LOG_TABLE FROM ALL_MVIEW_LOGS)\
							AND (A.OWNER, A.TABLE_NAME)\
							NOT IN (SELECT OWNER, TABLE_NAME FROM ALL_OBJECT_TABLES)");

	return table_info_query;
}



