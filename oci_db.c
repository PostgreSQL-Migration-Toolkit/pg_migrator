#include <stdio.h>
#include <signal.h>
#include <string.h>
#undef text
#include <oci.h>
#include "db.h"

#define ERROR_BUFF	512

static OCIEnv          *envhp;
static OCIServer       *srvhp;
static OCIError        *errhp;
static OCISvcCtx       *svchp;
static OCIStmt         *stmthp;
static OCIDefine       *defnp;
static OCISession      *authp;

text errbuf[ERROR_BUFF];

static void 
log_error(sword status)
{
	ub4 errcode = 0;

	switch (status)
	{
		case OCI_SUCCESS:
			break;
		case OCI_SUCCESS_WITH_INFO:
			break;
		case OCI_NEED_DATA:
			break;
		case OCI_NO_DATA:
			break;
		case OCI_ERROR:
			OCIErrorGet (errhp, 1, NULL, (sb4 *)&errcode, errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
			break;
		case OCI_INVALID_HANDLE:
			break;
		case OCI_STILL_EXECUTING:
			break;
		default:
			break;
	}
}

char *
get_db_log(void)
{
	return (char*)errbuf;
}

int
dbinit(void)
{
	char buf[2024];
	int connlen = snprintf(buf, sizeof buf,
	"(DESCRIPTION =\n"
	"  (ADDRESS_LIST =\n"
	"    (ADDRESS =\n"
	"      (PROTOCOL = TCP)\n"
	"      (HOST = %s)\n"
	"      (PORT = %s)\n"
	"    )\n"
	"  )\n"
	"  (CONNECT_DATA =\n"
	"    (SID = %s)\n"
	"  )\n"
	")", "127.0.0.1", "49161", "XE");

	OCIInitialize(OCI_DEFAULT, 0, 0, 0, 0);

	OCIEnvInit((OCIEnv **)&envhp, OCI_DEFAULT,0, 0);

	OCIHandleAlloc(envhp, (dvoid **) &errhp, OCI_HTYPE_ERROR, 0, 0);
	OCIHandleAlloc(envhp, (dvoid **) &srvhp, OCI_HTYPE_SERVER, 0, 0);
	OCIHandleAlloc(envhp, (dvoid **) &svchp, OCI_HTYPE_SVCCTX, 0, 0);

	OCIServerAttach(srvhp, errhp, (text *)buf, connlen, 0);
	OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, (dvoid *)srvhp, (ub4) 0, OCI_ATTR_SERVER, (OCIError *) errhp);
	return 0;
}

int
dbcleanup(void)
{
	if (stmthp)
		OCIHandleFree(stmthp, OCI_HTYPE_STMT);

	if (errhp)
		OCIServerDetach(srvhp, errhp, OCI_DEFAULT);

	if (srvhp)
		OCIHandleFree(srvhp, OCI_HTYPE_SERVER);
	
	if (svchp)
		OCIHandleFree((dvoid *) svchp, OCI_HTYPE_SVCCTX);
	
	if (errhp)
		OCIHandleFree((dvoid *) errhp, OCI_HTYPE_ERROR);
	return 0;
}


int
dbconnect(char *username, char *password)
{
	OCIHandleAlloc(envhp, (dvoid **)&authp, OCI_HTYPE_SESSION, 0, 0);
	OCIAttrSet(authp, OCI_HTYPE_SESSION, username, strlen(username), OCI_ATTR_USERNAME, errhp);
	OCIAttrSet(authp, OCI_HTYPE_SESSION, password, strlen(password), OCI_ATTR_PASSWORD, errhp);
	return 0;
}

int
dbbegin(void)
{
	int r = OCISessionBegin(svchp, errhp, authp, OCI_CRED_RDBMS, OCI_DEFAULT);
	if (r != 0)
		log_error(r);
	return r;
}

int
dbprepare(char *query)
{
	int r;

	OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, authp, 0, OCI_ATTR_SESSION, errhp);
	OCIHandleAlloc(envhp, (dvoid**)&stmthp, OCI_HTYPE_STMT, 0, 0);
	r = OCIStmtPrepare(stmthp, errhp, (const OraText *)query, strlen(query), OCI_NTV_SYNTAX, OCI_DEFAULT);
	if (r != 0)
		log_error(r);
	return r;
}
int
dbbind_string(void *v, int pos, int len)
{
	int r;
	int is_null;
    r = OCIDefineByPos(stmthp, &defnp, errhp, pos, (dvoid *) v, len, SQLT_STR, &is_null, 0, 0, OCI_DEFAULT);
	if (r < 0)
		log_error(r);
	return r;
}

int
dbbind_int(void *v, int pos)
{
	int r;

    r = OCIDefineByPos(stmthp, &defnp, errhp, pos, (dvoid *) v, sizeof(sword), SQLT_INT, 0, 0, 0, OCI_DEFAULT);
	if (r < 0)
		log_error(r);
	return r;
}

int
dbexecute(void)
{
	int r;

	r = OCIStmtExecute(svchp, stmthp, errhp, 1, 0, NULL, NULL, OCI_DEFAULT);
	if (r < 0)
		log_error(r);
	return r;
}

int
dbfetch(void)
{
	int r;

	r = OCIStmtFetch (stmthp, errhp, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
	if (r == OCI_NO_DATA)
		return 0;
	if (r < 0)
	{
		log_error(r);
		return -1;
	}
	return 1;
}

