/*
 * Copyright (C) 2012 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <time.h>
#include <sys/time.h>
#include <db.h>
#include "log_db.h"
#include "../common/common.h"
#include "../common/ts_print.h"



/*
 *
 * Constants and byte flags/masks
 *
 */

#define LOG_DB_ENV_DIR			"/rw/mx00/log/"
#define LOG_DB_FILE_NAME		"/rw/mx00/log/log.db"
#define	LOG_DB_LOAD_PROFILE		"load_profile"
#define	LOG_DB_CONSUM_ARCH		"consum_arch"

static DB_ENV *db_env = NULL;
static DB *db_load_profile = NULL;
static DB *db_consum_arch = NULL;

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
	DBT *key = NULL, *data = NULL;
	uint32_t *db_rec_arr = NULL;
	int i , fld_idx, ret;
	time_t cur_time = time(NULL);

	if(!db_req) return;

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Adding record to %s DB: Time = %d, Address = %d\n", db_req->dname,  (int)cur_time, adr);

    key = calloc(1, sizeof(DBT));
	data = calloc(1, sizeof(DBT));
	db_rec_arr = calloc(fld_num + 1, sizeof(uint32_t));

	if(key && data && db_rec_arr)
	{
		cur_time = htonl(cur_time);
		key->data = &cur_time;
		key->size = sizeof(cur_time);

		db_rec_arr[0] = htonl(adr);

		for(i=1;i<=fld_num;i++)
		{
			fld_idx = log_db_get_fld_idx(log_flds[i-1]);

			if(fld_idx >= 0)
			{
				db_rec_arr[i] = htonl(log_rec[fld_idx]);
			}
		}

		data->data = db_rec_arr;
		data->size = (fld_num + 1) * sizeof(uint32_t);

		ret = db_req->put(db_req, NULL, key, data, 0);

		if(ret != 0)
		{
		    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error adding record to %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);
		}
	}

	if(key) free(key);
	if(data) free(data);
	if(db_rec_arr) free(db_rec_arr);
}


void log_db_load_profile_add_rec(uint32_t adr, uint32_t *log_rec)
{
	log_db_add_var_rec(adr, log_rec, db_load_profile, log_load_profile_flds, LOG_LP_FLD_NUM);
}


void log_db_consum_arch_add_rec(uint32_t adr, uint32_t *log_rec)
{
	log_db_add_var_rec(adr, log_rec, db_consum_arch, log_consum_arch_flds, LOG_CA_FLD_NUM);
}


int log_db_open_db(DB_ENV *log_env, DB **log_db, char *file_name, char *db_name)
{
	int ret;

	if(*log_db) return -1;

	ret = db_create(log_db, log_env, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error creating %s handle: %s (%d)\n", db_name, db_strerror(ret), ret);
	    return -1;
	}

	ret = (*log_db)->set_lorder(*log_db, 4321); // set byte order to big-endian (it's just indicator!)
	ret = (*log_db)->set_flags(*log_db, DB_DUP); // allow duplicate keys in database
	ret = (*log_db)->open(*log_db, NULL, file_name, db_name, DB_BTREE, DB_CREATE , 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Database %s open failed: %s (%d)\n", db_name, db_strerror(ret), ret);
	    return -1;
	}

	return 0;
}


int log_db_env_open()
{
	int ret;

	if(db_env) return -1;

	ret = db_env_create(&db_env, 0);
	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error creating DB_ENV handle: %s (%d)\n", db_strerror(ret), ret);
	    return -1;
	}

	ret = db_env->open(db_env, LOG_DB_ENV_DIR, DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0);
	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - DB_ENV open failed: %s (%d)\n", db_strerror(ret), ret);
	    return -1;
	}

	ret = log_db_open_db(db_env, &db_load_profile, LOG_DB_FILE_NAME, LOG_DB_LOAD_PROFILE);
	if(ret != 0) goto err;

	ret = log_db_open_db(db_env, &db_consum_arch, LOG_DB_FILE_NAME, LOG_DB_CONSUM_ARCH);
	if(ret != 0) goto err;

	return 0;

err:
	if(db_consum_arch) db_consum_arch->close(db_consum_arch, 0);
	if(db_load_profile) db_load_profile->close(db_load_profile, 0);

	if(db_env) db_env->close(db_env, 0);

	return -1;
}


void log_db_env_close()
{
	ts_printf(STDOUT_FILENO, "IEC61850: BDB - Closing databases and environment\n");

	if(db_load_profile)
	{
		db_load_profile->close(db_load_profile, 0);
		db_load_profile = NULL;
	}
	if(db_consum_arch)
	{
		db_consum_arch->close(db_consum_arch, 0);
		db_consum_arch = NULL;
	}

	if(db_env)
	{
		db_env->close(db_env, 0);
		db_env = NULL;
	}
}










