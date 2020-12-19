/*
 *	pg_migrator.c
 *
 *	main source file
 *
 *	Copyright (c) 2010-2020, PostgreSQL Global Development Group
 */
#include "postgres.h"

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#include "access/transam.h"
#include "catalog/pg_class_d.h"
#include "common/file_perm.h"
#include "common/logging.h"
#include "common/restricted_token.h"
#include "fe_utils/string_utils.h"
#include "utils/hsearch.h"
#include "utils/inval.h"

#include "db.h"
#include "pg_migrator.h"

#define MAX_STRING 1024

static HTAB *hTableInfo = NULL;

typedef struct
{
	char table_name[MAX_STRING];
	char owner[MAX_STRING];
} keyTableInfo;

typedef struct
{
	keyTableInfo key;		/* hash key - must be first */
	char comment[MAX_STRING];
	char table_type[MAX_STRING];
} entryTableInfo;

typedef struct table_info
{
	char table_name[MAX_STRING];
	char owner[MAX_STRING];
	char comments[MAX_STRING];
	char table_type[MAX_STRING];
} table_info;

typedef struct table_detail_info
{
	char table_name[MAX_STRING];
	char owner[MAX_STRING];
	char tablespace[MAX_STRING];
	char comment[MAX_STRING];
	char table_type[MAX_STRING];
	bool nologging;
	bool partitioned;
	int fillfactor;
} table_detail_info;

table_info ti;
table_detail_info dti;

static int import_tables(void);
static int reterive_table_info(table_info *ti);
static int reterive_table_info_detail(table_detail_info *ti);

int
main(int argc, char **argv)
{
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
	
	prep_status("importing tables");
	r = import_tables();
	if (r < 0)
		goto exit;
	
	check_ok();
	prep_status("disconnecting from remote source session (%s)", get_source_dbhost());
	dbcleanup();
	check_ok();
	return 0;

exit:	
	dbcleanup();
	pg_log(PG_FATAL,"\npg_stat_monitor: %s\n\t%s", "failed to execute query", get_db_log());
	return 0;
}

static int
reterive_table_info(table_info *ti)
{
	int         r;
	HASHCTL     ctl;

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(keyTableInfo);
	ctl.entrysize = sizeof(entryTableInfo);
	hTableInfo = hash_create("pg_stat_monitor: table_info hash", 256, &ctl, HASH_ELEM | HASH_BLOBS);

	if (verbose() >= 2)
		pg_log(PG_REPORT,"\n---user-tables----\n%s\n---uesr-tables---\n", deparse_user_tables());
	
	r = dbbegin();
	if (r < 0)
		goto exit;

	r = dbprepare(deparse_user_tables());
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->table_name, 1, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->comments, 2, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->table_type, 3, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->owner, 4, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbexecute();
	if (r < 0)
		goto exit;
	
	for(;;)
	{
		if (verbose() >= 5)
			pg_log(PG_REPORT,"\n%s\t%s\t%s\t%s", ti->table_name, ti->comments, ti->table_type, ti->owner);
		
		if (verbose() >= 3)
			prep_status("\treteriving identity information of the table (%s)", ti->table_name);
		
		if (verbose() >= 3)
			check_ok();
		
		r = dbfetch();
		if (r <= 0)
			break;
	}
	return 0;

exit:
	// query cleanup
	return -1;
}

static int
reterive_table_info_detail(table_detail_info *ti)
{
	int r;
	
	r = dbbegin();
	if (r < 0)
		goto exit;

	if (verbose() >= 2)
		pg_log(PG_REPORT,"\n---user-table-details---\n%s\n---user-table-details---\n", deparse_user_table_details());
	
	r = dbprepare(deparse_user_table_details());
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->table_name, 1, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->owner, 2, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->tablespace, 3, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbbind_string(&ti->nologging, 4, MAX_STRING);
	if (r < 0)
		goto exit;

	r = dbbind_string(&ti->partitioned, 5, MAX_STRING);
	if (r < 0)
		goto exit;

	r = dbbind_string(&ti->fillfactor, 6, MAX_STRING);
	if (r < 0)
		goto exit;
	
	r = dbexecute();
	if (r < 0)
		goto exit;
	
	for(;;)
	{
		if (verbose() >= 5)
			pg_log(PG_REPORT,"\n%s\t%s\t%s\t%s", ti->table_name, ti->comment, ti->table_type, ti->owner);
		
		//hash_search(ti.table_name);	
		//ti.comment = key.comment;
		//ti.table_type = key.table_type
		r = dbfetch();
		if (r <= 0)
			break;
	}
	return 0;

exit:
	return -1;
}

int
import_tables(void)
{
	int r;
	
	if (verbose() >= 2)
		pg_log(PG_REPORT,"\n---import-tables---\n%s\n---import-tables---\n", deparse_user_tables());
		
	r = reterive_table_info(&ti);
	if (r < 0)
		goto exit;
	
	r = reterive_table_info_detail(&dti);
	if (r < 0)
		goto exit;
	
	return 0;

exit:
	// query cleanup
	return -1;
}
