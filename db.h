#include <stdio.h>
#include <signal.h>
#include <string.h>
#define text dbtext
#include <oci.h>
#undef text

char *get_db_log(void);
int dbinit(void);
int dbcleanup(void);
int dbconnect(char *username, char *password);
int dbbegin(void);
int dbprepare(char *query);
int dbbind_string(void *v, int pos, int len);
int dbbind_int(void *v, int pos);
int dbexecute(void);
int dbfetch(void);

