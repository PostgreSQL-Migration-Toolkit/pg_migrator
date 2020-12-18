/*
 *	guc.c
 *
 *	guc functions
 *
 *	Copyright (c) 2010-2019, PostgreSQL Global Development Group
 */

#include "postgres_fe.h"
#include "common/username.h"
#include <glib/gprintf.h>

#include "pg_migrator.h"

#define PG_MIGRATOR_CONFIG 	"pg_migrator.conf"

GKeyFile	*keyfile;

int
read_config_file(void)
{
 	GKeyFileFlags  flags;
 	GError         *error = NULL;

 	keyfile = g_key_file_new ();
 	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

 	if (!g_key_file_load_from_file (keyfile, PG_MIGRATOR_CONFIG, flags, &error))
 	{
		pg_log(PG_FATAL,"%s\" %s\"\n%s\n", "pg_migrator: could not access the configuration file", PG_MIGRATOR_CONFIG,  "No such file or directory");
   		return -1;
 	}
	return 0;
}

char*
get_source_guc(char *guc)
{
	char *guc_val;
	guc_val = g_key_file_get_string(keyfile, "SOURCE", guc, NULL);
	return guc_val;
}

char*
get_destination_guc(char *guc)
{
	char *guc_val;
	guc_val = g_key_file_get_string(keyfile, "DESTINATION", guc, NULL);
	return guc_val;
}

char*
get_source_type(void)
{
	char *source_type;
	source_type = get_source_guc("SOURCE_TYPE");

	if ((source_type == NULL) || (!((strcmp(source_type, "oracle") == 0) ||
				((strcmp(source_type, "mysql") == 0)))))
		pg_log(PG_FATAL,"%s\n%s\n", "pg_migrator: invalid source_type", "valid options are \"oracle\" and \"mysql\"");

	return source_type;
}

char*
get_source_dbusername(void)
{
	char *username;
	username = get_source_guc("USERNAME");
	
	if (username == NULL || strlen(username) == 0)
		pg_log(PG_FATAL,"%s\n", "pg_migrator: invalid or missing source database username");

	return username;
}

char*
get_source_dbpassword(void)
{
	char *pass;
	pass = get_source_guc("PASSWORD");
	if (pass == NULL || strlen(pass) == 0)
		pg_log(PG_FATAL,"%s\n", "pg_migrator: invalid or missing source database password");

	return pass;
}

char*
get_source_dbhost(void)
{
	char *host;
	host = get_source_guc("HOST");
	if (host == NULL || strlen(host) == 0)
		pg_log(PG_FATAL,"%s\n", "pg_migrator: invalid or missing source database host/ip");

	return host;
}
