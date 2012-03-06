/*
 * Copyright (C) 2012 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <time.h>
#include <sys/time.h>
#include "log_db.h"
#include "../common/common.h"
#include "../common/ts_print.h"
#include "../common/paths.h"

/*
 *
 * Constants and byte flags/masks
 *
 */

#define LOG_DB_FILE_NAME		"log.db"
#define	LOG_DB_EXPORT_LOG		"export_log"

static DB_ENV *db_env = NULL;
static DB *db_export_log = NULL;

char* log_fld_list[] = {
	"SupWh",
	"DmdWh",

	"SupVArh",
	"DmdVArh",

	"WphsA",
	"WphsB",
	"WphsC",
	"TotW",

	"VArphsA",
	"VArphsB",
	"VArphsC",
	"TotVAr",

	"VAphsA",
	"VAphsB",
	"VAphsC",
	"TotVA",

	"PhVphsA",
	"PhVphsB",
	"PhVphsC",

	"AphsA",
	"AphsB",
	"VphsC",

	"PFphsA",
	"PFphsB",
	"PFphsC"
};

char* log_load_profile_flds[] = {
	"SupWh",
	"DmdWh",

	"SupVArh",
	"DmdVArh",

	"WphsA",
	"WphsB",
	"WphsC",
	"TotW",

	"VArphsA",
	"VArphsB",
	"VArphsC",
	"TotVAr",

	"VAphsA",
	"VAphsB",
	"VAphsC",
	"TotVA",

	"PhVphsA",
	"PhVphsB",
	"PhVphsC",

	"AphsA",
	"AphsB",
	"VphsC",

	"PFphsA",
	"PFphsB",
	"PFphsC"
};

char* log_consum_arch_flds[] = {
	"SupWh",
	"DmdWh",

	"SupVArh",
	"DmdVArh"
};

log_db_serv_prm log_serv_prm_list[LOG_SERV_PRM_SIZE] =
{
	{
		"load_profile",
		NULL,
		log_load_profile_flds,
		25,
		60, // 1 minute
		0,
		86400, // 10800 // 3 hours //86400, // 24 hours
		0,
		86400 // 10800 // 3 hours //86400, // 24 hours
	},

	{
		"consum_arch",
		NULL,
		log_consum_arch_flds,
		4,
		1800, //1800, // 30 minutes
		0,
		604800, // 10800 // 3 hours //604800, // 1 week
		0,
		1552000 // 10800 // 3 hours //15552000 // 180 days
	}
};


/*
 *
 * Structures
 *
 */

/*
 *
 * Functions
 *
 */

int log_db_get_fld_idx(char *var_name)
{
	int i;
	for(i=0;i<LOG_FLD_NUM;i++)
	{
		if(strcmp(log_fld_list[i], var_name) == 0) return i;
	}

	return -1;
}


void log_db_add_var_rec(uint32_t adr, uint32_t *log_rec, DB *db_req, char **log_flds, int fld_num)
{
	DBT key, data;
	uint32_t db_rec_arr[fld_num+1];
	uint32_t cur_time_byte;
	int i , fld_idx, ret;
	time_t cur_time = time(NULL);

	if(!db_req) return;

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Adding record to %s DB: Time = %d, Address = %d\n", db_req->dname,  (int)cur_time, adr);

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    cur_time_byte = htonl( *((uint32_t*)&cur_time) );
	key.data = &cur_time_byte;
	key.size = sizeof(uint32_t);

	db_rec_arr[0] = htonl(adr);

	for(i=1;i<=fld_num;i++)
	{
		fld_idx = log_db_get_fld_idx(log_flds[i-1]);

		if(fld_idx >= 0)
		{
			db_rec_arr[i] = htonl(log_rec[fld_idx]);
		}
	}

	data.data = db_rec_arr;
	data.size = sizeof(uint32_t)*(fld_num+1);

	ret = db_req->put(db_req, NULL, &key, &data, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error adding record to %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);
	}
}


int log_db_open_db(DB_ENV *env_req, DB **db_req, char *file_name, char *db_name, uint8_t db_dup)
{
	int ret;

	if(*db_req) return -1;

	ret = db_create(db_req, env_req, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error creating %s handle: %s (%d)\n", db_name, db_strerror(ret), ret);
	    return -1;
	}

	ret = (*db_req)->set_lorder(*db_req, 4321); // set byte order to big-endian (it's just indicator!)
	if (db_dup) ret = (*db_req)->set_flags(*db_req, DB_DUP); // allow duplicate keys in database if needed
	ret = (*db_req)->open(*db_req, NULL, file_name, db_name, DB_BTREE, DB_CREATE, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Database %s open failed: %s (%d)\n", db_name, db_strerror(ret), ret);
	    return -1;
	}
	else
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Database %s opened with success\n", db_name);


	return 0;
}


int log_db_env_open()
{
	int ret, i;
	struct stat sb;
	char *fname;

	if(db_env) return -1;

	// check if directory exists
	if (stat(getpath2log(), &sb) != 0)
	{
		// if not - try to create one
		ts_printf(STDOUT_FILENO, "IEC61850: BDB - directory \"%s\" does not exist\n", getpath2log());

		if (mkdir(getpath2log(), S_IRWXU) != 0)
		{
			// directory creation failed
			ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cannot mkdir \"%s\" (%s)\n", getpath2log(), strerror(errno));
			goto err;
		}

		ts_printf(STDOUT_FILENO, "IEC61850: BDB - Directory \"%s\" created with success\n", getpath2log());
	}

	ret = db_env_create(&db_env, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error creating DB_ENV handle: %s (%d)\n", db_strerror(ret), ret);
	    goto err;
	}

	ret = db_env->open(db_env, getpath2log(), DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0);
	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - DB_ENV open failed: %s (%d)\n", db_strerror(ret), ret);
	    goto err;
	}
	else
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - DB_ENV opened with success: %s \n", getpath2log());

	ret = log_db_open_db(db_env, &db_export_log, LOG_DB_FILE_NAME, LOG_DB_EXPORT_LOG, 0);
	if(ret != 0) goto err;

	for(i=0; i<LOG_SERV_PRM_SIZE; i++)
	{
		ret = log_db_open_db(db_env, &log_serv_prm_list[i].db_req, LOG_DB_FILE_NAME, log_serv_prm_list[i].db_name, 1);
		if(ret != 0) goto err;
	}

	return 0;

err:
	log_db_env_close();
	return -1;
}


void log_db_env_close()
{
	int i;

	if(!db_env) return;

	ts_printf(STDOUT_FILENO, "IEC61850: BDB - Closing DB environment\n");

	if(db_export_log)
	{
		db_export_log->close(db_export_log, 0);
		db_export_log = NULL;
	}

	for(i=0; i<LOG_SERV_PRM_SIZE; i++)
	{
		if(log_serv_prm_list[i].db_req)
		{
			log_serv_prm_list[i].db_req->close(log_serv_prm_list[i].db_req, 0);
			log_serv_prm_list[i].db_req = NULL;
		}
	}

	if(db_env)
	{
		db_env->close(db_env, 0);
		db_env = NULL;
	}
}


time_t log_db_get_export_time(DB *db_req)
{
	DBT key, data;
	DBC *cursor = NULL;
	int ret;
	time_t exp_time = 0;

	if(!db_env || !db_req || !db_export_log) return -1;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    ret = db_export_log->cursor(db_export_log, NULL, &cursor, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cursor setup failed for %s DB: %s (%d)\n", db_export_log->dname, db_strerror(ret), ret);

	    return -1;
	}

    // get export log records in the loop
	while((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0)
    {
		if(strcmp((char*)key.data, db_req->dname) == 0)
		{
			exp_time = ntohl( *((uint32_t*)data.data) );
			break;
		}
    }

	cursor->c_close(cursor);

	return exp_time;
}


void log_db_set_export_time(DB *db_req, time_t export_time)
{
	DBT key, data;
	uint32_t export_time_byte;
	int ret;

	if(!db_req || !db_req || !db_export_log) return;

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Set last export time for %s DB: Time = %d\n", db_req->dname,  (int)export_time);

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

	key.data = db_req->dname;
	key.size = strlen(db_req->dname)+1;

	export_time_byte = htonl( *((uint32_t*)&export_time) );
	data.data = &export_time_byte;
	data.size = sizeof(uint32_t);

	ret = db_export_log->put(db_export_log, NULL, &key, &data, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error adding record to %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);
	}

	return;
}


int log_db_export_data(DB *db_req, time_t cut_time, char *db_file_out)
{
	DBT key, data;
	DBC *cursor = NULL;
	DB *export_db = NULL;
	time_t prev_exp_time, rec_time;
	int ret, rec_count_exp = 0;

	if(!db_env || !db_req || !db_file_out) return -1;

	ts_sprintf(db_file_out, "%s%s%s%d%s", getpath2log(), db_req->dname, "_", cut_time, ".db");

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    ret = db_req->cursor(db_req, NULL, &cursor, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cursor setup failed for %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);

	    return -1;
	}

	ret = log_db_open_db(NULL, &export_db, db_file_out, db_req->dname, 1);

	if(ret == -1)
	{
		cursor->c_close(cursor);
		return ret;
	}

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Start creation of export file \"%s\"\n", db_file_out);

    prev_exp_time = log_db_get_export_time(db_req);

    // get log records in the loop
	while((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0)
    {
		rec_time = ntohl( *((uint32_t*)key.data) );

		if( prev_exp_time < rec_time && rec_time <= cut_time)
    	{
    		ret = export_db->put(export_db, NULL, &key, &data, 0);

    		if(ret == 0)
    			rec_count_exp++;
    		else
    			break;
    	}
    }

	cursor->c_close(cursor);
	cursor = NULL;

	export_db->close(export_db, 0);
	export_db = NULL;

	if((ret == 0 || DB_NOTFOUND) && rec_count_exp > 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Export file \"%s\" created with success. rec_count_exp = %d\n", db_file_out, rec_count_exp);

	    log_db_set_export_time(db_req, cut_time);
	}
	else
	{
		remove(db_file_out);
		ret = -1;
	}

	return ret;
}


int log_db_clean_old_data(DB *db_req, time_t cut_time, time_t storage_deep)
{
	DBT key, data;
	DBC *cursor = NULL;
	time_t rec_time;
	int ret, rec_count_del = 0;

	if(!db_env || !db_req) return -1;

	cut_time -= storage_deep;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

	// open cursor
    ret = db_req->cursor(db_req, NULL, &cursor, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cursor setup failed for %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);
	}
	else
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Start cleaning old records from %s DB\n", db_req->dname);

	    // delete log records in the loop
		while((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0)
	    {
			rec_time = ntohl( *((uint32_t*)key.data) );

	    	if(rec_time <= cut_time)
	    	{
	    		ret = cursor->c_del(cursor, 0);
	    		if(ret == 0)
	    			rec_count_del++;
	    		else
	    			break;
	    	}
	    }

		cursor->c_close(cursor);
		cursor = NULL;

		if((ret == 0 || DB_NOTFOUND) && rec_count_del > 0)
		{
		    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Old records from %s DB deleted with success. rec_count_del = %d\n", db_req->dname, rec_count_del);

		    db_req->compact(db_req, NULL, NULL, NULL, NULL, DB_FREE_SPACE, NULL);
		}
	}

	return ret;
}


void log_db_init_timers()
{
	int i;
	time_t sys_time = time(NULL), prev_exp_time;

	for(i=0; i<LOG_SERV_PRM_SIZE; i++)
	{
		log_serv_prm_list[i].add_timer = sys_time;

		prev_exp_time = log_db_get_export_time(log_serv_prm_list[i].db_req);

		if(prev_exp_time > 0)
			log_serv_prm_list[i].export_timer = prev_exp_time;
		else
		{
			log_db_set_export_time(log_serv_prm_list[i].db_req, sys_time);
			log_serv_prm_list[i].export_timer = sys_time;
		}
	}
}






