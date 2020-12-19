/*
 *	deparce.c
 *
 *	deparce source file
 *
 *	Copyright (c) 2010-2020, PostgreSQL Global Development Group
 */

#include "postgres_fe.h"
#include "pg_migrator.h"

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#include "db.h"

#define QUERY_LEN	2048

char query[QUERY_LEN]; 
char sub_query[QUERY_LEN]; 

char*
deparse_system_users(void)
{
	snprintf(sub_query, QUERY_LEN, "%s", 
	"SELECT	username\n\
	FROM	dba_users\n\
	WHERE	username NOT IN ('ANONYMOUS'\n\
	\t,'APEX_040200', 'APEX_PUBLIC_USER', 'APPQOSSYS', 'AUDSYS', 'BI'\n\
	\t,'CTXSYS', 'DBSNMP', 'DIP', 'DVF', 'DVSYS', 'EXFSYS', 'FLOWS_FILES'\n\
	\t,'GSMADMIN_INTERNAL', 'GSMCATUSER', 'GSMUSER', 'HR', 'IX', 'LBACSYS'\n\
	\t,'MDDATA', 'MDSYS', 'OE', 'ORACLE_OCM', 'ORDDATA', 'ORDPLUGINS'\n\
	\t,'ORDSYS', 'OUTLN', 'PM', 'SCOTT', 'SH', 'SI_INFORMTN_SCHEMA'\n\
	\t,'SPATIAL_CSW_ADMIN_USR', 'SPATIAL_WFS_ADMIN_USR','SYS','SYSBACKUP'\n\
	\t,'SYSDG', 'SYSKM', 'SYSTEM', 'WMSYS', 'XDB', 'SYSMAN', 'RMAN', 'RMAN_BACKUP'\n\
	\t,'OWBSYS', 'OWBSYS_AUDIT', 'APEX_030200','MGMT_VIEW', 'MGMT_VIEW', 'OJVMSYS')");

	return sub_query;
}

char*
deparse_user_tables(void)
{
	memset(query, 0, QUERY_LEN);
	snprintf(query, QUERY_LEN, "%s \n(\n\t%s\n) %s", 
"SELECT	A.TABLE_NAME,\n\
	A.COMMENTS,\n\
	A.TABLE_TYPE,\n\
	A.OWNER\n\
FROM\n\
	ALL_TAB_COMMENTS A,\n\
	ALL_OBJECTS O\n\
WHERE\n\
	A.OWNER = O.OWNER\n\
AND A.TABLE_NAME = O.OBJECT_NAME\n\
AND O.OBJECT_TYPE = 'TABLE'\n\
AND A.OWNER IN ", deparse_system_users(),"\n\
AND (A.OWNER, A.TABLE_NAME) NOT IN (SELECT OWNER, MVIEW_NAME FROM ALL_MVIEWS UNION ALL SELECT LOG_OWNER, LOG_TABLE FROM ALL_MVIEW_LOGS)\n\
AND (A.OWNER, A.TABLE_NAME) NOT IN (SELECT OWNER, TABLE_NAME FROM ALL_OBJECT_TABLES)");
	return query;
}

char*
deparse_user_table_details(void)
{
	memset(query, 0, QUERY_LEN);
	snprintf(query, QUERY_LEN, "%s \n(\n\t%s\n) %s", 
"SELECT A.OWNER,\n\
	A.TABLE_NAME,\n\
	NVL(num_rows,1) NUMBER_ROWS,\n\
	A.TABLESPACE_NAME,\n\
	A.NESTED,\n\
	A.LOGGING,\n\
	A.PARTITIONED,\n\
	A.PCT_FREE\n\
FROM DBA_TABLES A, ALL_OBJECTS O\n\
WHERE A.OWNER=O.OWNER\n\
AND A.TABLE_NAME = O.OBJECT_NAME\n\
AND O.OBJECT_TYPE = 'TABLE'\n\
AND A.OWNER NOT IN ", deparse_system_users(), "\n\ 
AND A.TEMPORARY='N'\n\
AND (A.NESTED != 'YES' OR A.LOGGING != 'YES')\n\
AND A.SECONDARY = 'N'\n\
AND (A.DROPPED IS NULL OR A.DROPPED = 'NO')\n\
AND (A.OWNER, A.TABLE_NAME)\n\
NOT IN (SELECT OWNER, MVIEW_NAME FROM ALL_MVIEWS UNION ALL SELECT LOG_OWNER, LOG_TABLE FROM ALL_MVIEW_LOGS)\n\
AND (A.OWNER, A.TABLE_NAME) NOT IN (SELECT OWNER, TABLE_NAME FROM ALL_OBJECT_TABLES)\n\
AND (A.IOT_TYPE IS NULL OR A.IOT_TYPE = 'IOT'");
	return query;
}


