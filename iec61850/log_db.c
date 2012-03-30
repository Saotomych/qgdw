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

#define LOG_DB_CFG_NAME			"logdb.cfg"
#define LOG_DB_FILE_NAME		"log.bdb"
#define	LOG_DB_LOAD_PROFILE		"load_profile"
#define	LOG_DB_CONSUM_ARCH		"consum_arch"
#define	LOG_DB_DEV_EVENT		"dev_event_log"
#define	LOG_DB_APP_EVENT		"app_event_log"
#define	LOG_DB_EXPORT			"export_log"

#define DB_LOG_SYNC_PERIOD				600			// 10 minutes
#define DB_LOG_LOAD_PROFILE_ADD			60			// 1 minute
#define DB_LOG_LOAD_PROFILE_EXPORT		86400		// 24 hours
#define DB_LOG_LOAD_PROFILE_DEEP		86400		// 24 hours
#define DB_LOG_CONSUM_ARCH_ADD			1800		// 30 minutes
#define DB_LOG_CONSUM_ARCH_EXPORT		86400		// 24 hours
#define DB_LOG_CONSUM_ARCH_DEEP			15552000 	// 180 days
#define DB_LOG_EVENT_EXPORT				86400		// 24 hours
#define DB_LOG_EVENT_DEEP				10368000 	// 120 days


static DB_ENV *db_env = NULL;
static DB *db_export_log = NULL;

log_db load_profile_db;
log_db consum_arch_db;
log_db dev_event_db;
log_db app_event_db;

static log_db *actdb = NULL;

static time_t db_sync_period = DB_LOG_SYNC_PERIOD;
static time_t db_sync_timer = 0;

static pthread_mutex_t db_log_mtx = PTHREAD_MUTEX_INITIALIZER;

static uint8_t end_script;


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

extern int get_dobj_idx(char *dobj_name);


void log_db_add_var_rec(log_db *db_req, uint32_t adr, uint32_t *log_rec);
int log_db_get_var(log_db *db_req, uint32_t adr, char *var_name, int len, time_t *log_time, uint32_t *value);
void log_db_add_event(log_db *db_req, uint32_t adr, char *msg, int len);
int log_db_get_event(log_db *db_req, uint32_t adr, time_t *rec_time, char **msg, int *len);


int log_db_get_fld_idx(log_db *db_req, const char *var_name)
{
	int i;

	for(i=0; i<db_req->flds_num; i++)
	{
		if(strcmp(db_req->flds_list+i*DOBJ_NAMESIZE, var_name) == 0) return i;
	}
	return -1;
}


int log_db_add_fld(log_db *db_req, const char *var_name)
{
	int ret;
	char *fld_new = NULL;

	ret = log_db_get_fld_idx(db_req, var_name);

	if(ret != -1) return 0;

	fld_new = (char*) realloc((void*) db_req->flds_list, DOBJ_NAMESIZE * (db_req->flds_num + 1));

	if(!fld_new) return -1;

	db_req->flds_list = fld_new;
	strcpy(db_req->flds_list + db_req->flds_num*DOBJ_NAMESIZE, var_name);
	db_req->flds_num++;

	return 0;
}


typedef struct _XML_Name{
	char *Name;
	void(*Function)(const char *pTag);
} XML_Name;


void log_db_xml_dbsync_tag(const char *pTag);
void log_db_xml_database_tag(const char *pTag);
void log_db_xml_var_tag(const char *pTag);
void log_db_xml_end_tag(const char *pTag);


static const XML_Name XTags[] = {
	{"DBSync", log_db_xml_dbsync_tag},
	{"Database", log_db_xml_database_tag},
	{"Var", log_db_xml_var_tag},
	{"/LogConfig", log_db_xml_end_tag}
};


void log_db_open_tag(const char *pS)
{
	const char *pT=pS;
	u08 s, i;

	while(*pS < 'A') pS++;
	pS--;
	if ((*pS != '?') && (*pS != '/')) pS++;

	for(i = 0; i < sizeof(XTags)/sizeof(XML_Name); i++)
	{
	    s=0; pS=pT;
	    while(*pS == XTags[i].Name[s])
	    {
	        pS++; s++;
	    }
	    if((XTags[i].Name[s] == 0) && (*pS <= 'A'))
	    {
	    	XTags[i].Function(pS);
	    	break;
	    }
	}
}


void log_db_xml_parser(const char *XMLScript)
{
	const char *pS=XMLScript;

	end_script = 0;
	while (!end_script)
	{
		if (*(pS++) == '<') log_db_open_tag(pS);
	}
}


int log_db_config_build(const char *filename)
{
	char *config_file;
	FILE *fcid;
	int cfglen, ret = 0;
	struct stat fst;

	// Get size of main config file
	if (stat(filename, &fst) == -1)
	{
		ts_printf(STDOUT_FILENO, "IEC61850: Log configuration file not found\n");
		return -1;
	}

	config_file = malloc(fst.st_size + 1);

	// Loading main config file
	fcid = fopen(filename, "r");
	cfglen = fread(config_file, 1, (size_t) (fst.st_size), fcid);
	fclose(fcid);
	config_file[fst.st_size] = '\0'; // make it null terminating string
	if(!strstr(config_file, "</LogConfig>"))
	{
		ts_printf(STDOUT_FILENO, "IEC61850: Log configuration file is incomplete\n");
		free(config_file);
		return -1;
	}
	else
		ts_printf(STDOUT_FILENO, "IEC61850: Log configuration file parsing started\n");


	if(cfglen == fst.st_size)
		log_db_xml_parser(config_file);
	else
		ret = -1;

	free(config_file);

	return ret;
}


// In: p - pointer to string
// Out: key - pointer to string with key of SCL file
//      par - pointer to string par for this key without ""
char* log_db_get_next_parameter(char *p, char **key, char **par)
{
	int mode=0;

	*key = p;
	while (*p != '>')
	{
		if (!mode)
		{
			// Key mode
			if (*p == '='){
				mode = 1;	// switch to par mode
				*p = 0;
			}
		}
		else
		{
			// Par mode
			if (*p == '"'){
				if (mode == 2){
					*p = 0;
					return p+1;		// key + par ready
				}
				if (mode == 1){
					mode = 2;
					*par = p+1;
				}
			}
		}
		p++;
	}

	return 0;
}


void log_db_xml_dbsync_tag(const char *pTag)			// call parse DBSync
{
	char *p;
	char *key=0, *par=0;

	p = (char*) pTag;
	do
	{
		p = log_db_get_next_parameter(p, &key, &par);
		if(p && strstr((char*) key, "period")) db_sync_period = atoi(par);
	}
	while(p);

	ts_printf(STDOUT_FILENO, "IEC61850: Database synchronization period = %d\n", db_sync_period);
}


void log_db_xml_database_tag(const char *pTag)
{
	char *p;
	char *key=0, *par=0;

	p = (char*) pTag;
	do
	{
		p = log_db_get_next_parameter(p, &key, &par);
		if(p)
		{
			if(strstr((char*) key, "name"))
			{
				if(strstr(par, LOG_DB_LOAD_PROFILE)) actdb = &load_profile_db;
				else if(strstr(par, LOG_DB_CONSUM_ARCH)) actdb = &consum_arch_db;
				else if(strstr(par, LOG_DB_DEV_EVENT)) actdb = &dev_event_db;
				else if(strstr(par, LOG_DB_APP_EVENT)) actdb = &app_event_db;
			}
			else if(strstr((char*) key, "add_period") && actdb)
				actdb->add_period = atoi(par);
			else if(strstr((char*) key, "export_period") && actdb)
				actdb->export_period = atoi(par);
			else if(strstr((char*) key, "storage_deep") && actdb)
				actdb->storage_deep = atoi(par);
		}
	}
	while(p);

	if(actdb && actdb->db) ts_printf(STDOUT_FILENO, "IEC61850: Database \"%s\" parameters: add_period = %d, export_peiod = %d, storage_deep = %d\n", actdb->db->dname, actdb->add_period, actdb->export_period, (int)actdb->storage_deep);
}


void log_db_xml_var_tag(const char *pTag)
{
	char *p;
	char *key=0, *par=0;

	p = (char*) pTag;
	do
	{
		p = log_db_get_next_parameter(p, &key, &par);

		if(p && strstr((char*) key, "name") && actdb) log_db_add_fld(actdb, par);
	}
	while(p);

	if(actdb) ts_printf(STDOUT_FILENO, "IEC61850: Variable \"%s\" for database \"%s\" added\n", par, actdb->db->dname);
}


void log_db_xml_end_tag(const char *pTag)
{
	end_script=1;
	ts_printf(STDOUT_FILENO, "IEC61850: Stop Log configuration file to parse\n");
}


void log_db_config_read(const char *file_name)
{
	load_profile_db.flds_list = NULL;
	load_profile_db.flds_num = 0;
	load_profile_db.add_period = DB_LOG_LOAD_PROFILE_ADD;
	load_profile_db.add_timer = 0;
	load_profile_db.export_period = DB_LOG_LOAD_PROFILE_EXPORT;
	load_profile_db.export_timer = 0;
	load_profile_db.storage_deep = DB_LOG_LOAD_PROFILE_DEEP;
	load_profile_db.add_var_rec = log_db_add_var_rec;
	load_profile_db.get_var = log_db_get_var;
	load_profile_db.add_event = log_db_add_event;
	load_profile_db.get_event = log_db_get_event;

	consum_arch_db.flds_list = NULL;
	consum_arch_db.flds_num = 0;
	consum_arch_db.add_period = DB_LOG_CONSUM_ARCH_ADD;
	consum_arch_db.add_timer = 0;
	consum_arch_db.export_period = DB_LOG_CONSUM_ARCH_EXPORT;
	consum_arch_db.export_timer = 0;
	consum_arch_db.storage_deep = DB_LOG_CONSUM_ARCH_DEEP;
	consum_arch_db.add_var_rec = log_db_add_var_rec;
	consum_arch_db.get_var = log_db_get_var;
	consum_arch_db.add_event = log_db_add_event;
	consum_arch_db.get_event = log_db_get_event;

	dev_event_db.flds_list = NULL;
	dev_event_db.flds_num = 0;
	dev_event_db.add_period = 0;
	dev_event_db.add_timer = 0;
	dev_event_db.export_period = DB_LOG_EVENT_EXPORT;
	dev_event_db.export_timer = 0;
	dev_event_db.storage_deep = DB_LOG_EVENT_DEEP;
	dev_event_db.add_var_rec = log_db_add_var_rec;
	dev_event_db.get_var = log_db_get_var;
	dev_event_db.add_event = log_db_add_event;
	dev_event_db.get_event = log_db_get_event;

	app_event_db.flds_list = NULL;
	app_event_db.flds_num = 0;
	app_event_db.add_period = 0;
	app_event_db.add_timer = 0;
	app_event_db.export_period = DB_LOG_EVENT_EXPORT;
	app_event_db.export_timer = 0;
	app_event_db.storage_deep = DB_LOG_EVENT_DEEP;
	app_event_db.add_var_rec = log_db_add_var_rec;
	app_event_db.get_var = log_db_get_var;
	app_event_db.add_event = log_db_add_event;
	app_event_db.get_event = log_db_get_event;

	log_db_config_build(file_name);
}


void log_db_add_var_rec(log_db *db_req, uint32_t adr, uint32_t *log_rec)
{
	DBT key, data;
	uint32_t db_rec_arr[db_req->flds_num+1];
	uint32_t cur_time_byte;
	int i, fld_idx, ret;
	time_t cur_time = time(NULL);

	if(!db_env || !db_req || !db_req->db || db_req->flds_num == 0 || !log_rec) return;

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Adding record to %s DB: Time = %d, Address = %d\n", db_req->db->dname,  (int)cur_time, adr);

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    cur_time_byte = htonl( *((uint32_t*)&cur_time) );
	key.data = &cur_time_byte;
	key.size = sizeof(uint32_t);

	db_rec_arr[0] = htonl(adr);

	for(i=1;i<=db_req->flds_num;i++)
	{
		fld_idx = get_dobj_idx(db_req->flds_list + (i-1)*DOBJ_NAMESIZE);

		if(fld_idx >= 0)
		{
			db_rec_arr[i] = htonl(log_rec[fld_idx]);
		}
	}

	data.data = db_rec_arr;
	data.size = sizeof(uint32_t)*(db_req->flds_num+1);


	pthread_mutex_lock(&db_log_mtx);
	ret = db_req->db->put(db_req->db, NULL, &key, &data, 0);
	pthread_mutex_unlock(&db_log_mtx);


	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error adding record to %s DB: %s (%d)\n", db_req->db->dname, db_strerror(ret), ret);
	}
}


void log_db_add_event_ll(DB *db_req, void *buf, int len)
{
	DBT key, data;
	int ret;
	uint32_t cur_time_byte;

	time_t cur_time = time(NULL);

	if(!db_req) return;

	ts_printf(STDOUT_FILENO, "IEC61850: BDB - Adding record to %s DB: Time = %d\n", db_req->dname, (int)cur_time);

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    cur_time_byte = htonl( *((uint32_t*)&cur_time) );
	key.data = &cur_time_byte;
	key.size = sizeof(uint32_t);

	data.data = buf;
	data.size = len;

	pthread_mutex_lock(&db_log_mtx);
	ret = db_req->put(db_req, NULL, &key, &data, 0);
	pthread_mutex_unlock(&db_log_mtx);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error adding record to %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);
	}

	return;
}


void log_db_add_event(log_db *db_req, uint32_t adr, char *msg, int len)
{
	unsigned char *buf = NULL;

	buf = (unsigned char *) malloc(sizeof(uint32_t) + len);

	if(!buf) return;

	adr = htonl(adr);

	memcpy((void*)buf, (void*)&adr, sizeof(uint32_t));
	memcpy((void*)(buf+sizeof(uint32_t)), (void*)msg, len);

	log_db_add_event_ll(db_req->db, (void*)buf, sizeof(uint32_t) + len);

	free(buf);
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
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Database %s opened\n", db_name);

	return 0;
}


void log_db_close_db(log_db *db_req , DB_ENV *env_req)
{
	if(db_req->db)
	{
		db_req->db->close(db_req->db, 0);
		db_req->db = NULL;
	}

	if(db_req->flds_list)
	{
		free(db_req->flds_list);
		db_req->flds_list = NULL;
		db_req->flds_num = 0;
	}

	db_req->add_period = 0;
	db_req->add_timer = 0;
	db_req->export_period = 0;
	db_req->export_timer = 0;
	db_req->storage_deep = 0;
}


int log_db_env_open()
{
	int ret;
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

		ts_printf(STDOUT_FILENO, "IEC61850: BDB - Directory \"%s\" created\n", getpath2log());
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
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - DB_ENV opened: %s \n", getpath2log());

	ret = log_db_open_db(db_env, &load_profile_db.db, LOG_DB_FILE_NAME, LOG_DB_LOAD_PROFILE, 1);
	if(ret != 0) goto err;

	ret = log_db_open_db(db_env, &consum_arch_db.db, LOG_DB_FILE_NAME, LOG_DB_CONSUM_ARCH, 1);
	if(ret != 0) goto err;

	ret = log_db_open_db(db_env, &dev_event_db.db, LOG_DB_FILE_NAME, LOG_DB_DEV_EVENT, 0);
	if(ret != 0) goto err;

	ret = log_db_open_db(db_env, &app_event_db.db, LOG_DB_FILE_NAME, LOG_DB_APP_EVENT, 0);
	if(ret != 0) goto err;

	ret = log_db_open_db(db_env, &db_export_log, LOG_DB_FILE_NAME, LOG_DB_EXPORT, 0);
	if(ret != 0) goto err;

	fname = malloc(strlen(getpath2configs()) + strlen(LOG_DB_CFG_NAME) + 1);
	strcpy(fname, getpath2configs());
	strcat(fname, LOG_DB_CFG_NAME);

	log_db_config_read(fname);

	free(fname);

	return 0;

err:
	log_db_env_close();
	return -1;
}


void log_db_env_close()
{
	if(!db_env) return;

	ts_printf(STDOUT_FILENO, "IEC61850: BDB - Closing DB environment\n");

	pthread_mutex_lock(&db_log_mtx);

	log_db_close_db(&load_profile_db, db_env);
	log_db_close_db(&consum_arch_db, db_env);
	log_db_close_db(&dev_event_db, db_env);
	log_db_close_db(&app_event_db, db_env);

	if(db_export_log)
	{
		db_export_log->close(db_export_log, 0);
		db_export_log = NULL;
	}

	if(db_env)
	{
		db_env->close(db_env, 0);
		db_env = NULL;
	}

	pthread_mutex_unlock(&db_log_mtx);
}


time_t log_db_get_export_time(DB *db_req)
{
	DBT key, data;
	int ret;
	time_t exp_time = 0;

	if(!db_env || !db_req || !db_export_log) return -1;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = db_req->dname;
	key.size = strlen(db_req->dname) + 1;

	ret = db_export_log->get(db_export_log, NULL, &key, &data, 0);

	if(ret == 0) exp_time = ntohl( *((uint32_t*)data.data) );

	return exp_time;
}


void log_db_set_export_time(DB *db_req, time_t export_time)
{
	DBT key, data;
	uint32_t export_time_byte;
	int ret;

	if(!db_req || !db_req || !db_export_log) return;

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Set last export time for %s DB: Time = %d\n", db_req->dname, (int)export_time);

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

	key.data = db_req->dname;
	key.size = strlen(db_req->dname)+1;

	export_time_byte = htonl( *((uint32_t*)&export_time) );
	data.data = &export_time_byte;
	data.size = sizeof(uint32_t);


	pthread_mutex_lock(&db_log_mtx);
	ret = db_export_log->put(db_export_log, NULL, &key, &data, 0);
	pthread_mutex_unlock(&db_log_mtx);


	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Error adding record to %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);
	}

	return;
}


int log_db_export_data(DB *db_req, time_t cut_time)
{
	DBT key, data;
	DBC *cursor = NULL;
	DB *export_db = NULL;
	time_t prev_exp_time, rec_time;
	char db_file_tmp[64], db_file_out[64];
	int ret, rec_count_exp = 0;

	if(!db_env || !db_req) return -1;

	ts_sprintf(db_file_tmp, "%s%s%s%d%s", getpath2log(), db_req->dname, "_", cut_time, ".part");
	ts_sprintf(db_file_out, "%s%s%s%d%s", getpath2log(), db_req->dname, "_", cut_time, ".db");

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    ret = db_req->cursor(db_req, NULL, &cursor, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cursor setup failed for %s DB: %s (%d)\n", db_req->dname, db_strerror(ret), ret);

	    return -1;
	}

	ret = log_db_open_db(NULL, &export_db, db_file_tmp, db_req->dname, 1);

	if(ret == -1)
	{
		cursor->c_close(cursor);
		return ret;
	}

    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Start creation of export file \"%s\"\n", db_file_tmp);

    prev_exp_time = log_db_get_export_time(db_req);

	pthread_mutex_lock(&db_log_mtx);

	// get log records in the loop
	while((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0)
    {
		rec_time = ntohl( *((uint32_t*)key.data) );

		if(prev_exp_time < rec_time && rec_time <= cut_time)
    	{
    		ret = export_db->put(export_db, NULL, &key, &data, 0);

    		if(ret == 0)
    			rec_count_exp++;
    		else
    			break;
    	}
    }

	pthread_mutex_unlock(&db_log_mtx);

	cursor->c_close(cursor);
	cursor = NULL;

	export_db->close(export_db, 0);
	export_db = NULL;

	if((ret == 0 || DB_NOTFOUND) && rec_count_exp > 0)
	{
		rename(db_file_tmp, db_file_out);

	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Export file \"%s\" created. rec_count_exp = %d\n", db_file_out, rec_count_exp);

	    log_db_set_export_time(db_req, cut_time);

	    ret = rec_count_exp;
	}
	else
	{
		remove(db_file_tmp);
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

		pthread_mutex_lock(&db_log_mtx);

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

		pthread_mutex_unlock(&db_log_mtx);

		cursor->c_close(cursor);
		cursor = NULL;

		if((ret == 0 || DB_NOTFOUND) && rec_count_del > 0)
		{
		    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Old records from %s DB deleted. rec_count_del = %d\n", db_req->dname, rec_count_del);

		    db_req->compact(db_req, NULL, NULL, NULL, NULL, DB_FREE_SPACE, NULL);

		    ret = rec_count_del;
		}
	}

	return ret;
}


void log_db_init_timer(log_db *db_req, time_t init_time)
{
	time_t prev_exp_time;

	if(!db_env || !db_req) return;

	if(db_req->add_period > 0) db_req->add_timer = init_time;

	prev_exp_time = log_db_get_export_time(db_req->db);

	if(prev_exp_time > 0)
		db_req->export_timer = prev_exp_time;
	else
	{
		log_db_set_export_time(db_req->db, init_time);
		db_req->export_timer = init_time;
	}
}


void log_db_init_timers(time_t init_time)
{
	if(!db_env) return;

	log_db_init_timer(&load_profile_db, init_time);
	log_db_init_timer(&consum_arch_db, init_time);
	log_db_init_timer(&dev_event_db, init_time);
	log_db_init_timer(&app_event_db, init_time);

	db_sync_timer = init_time;
}


void log_db_file_sync()
{
	if(!db_env) return;

	pthread_mutex_lock(&db_log_mtx);

	if(load_profile_db.db) load_profile_db.db->sync(load_profile_db.db, 0);

	if(consum_arch_db.db) consum_arch_db.db->sync(consum_arch_db.db, 0);

	if(dev_event_db.db) dev_event_db.db->sync(dev_event_db.db, 0);

	if(app_event_db.db) app_event_db.db->sync(app_event_db.db, 0);

	if(db_export_log) db_export_log->sync(db_export_log, 0);

	pthread_mutex_unlock(&db_log_mtx);
}


void log_db_maintain_database(log_db *db_req, time_t in_time)
{
	if(db_req->export_timer > 0 && difftime(in_time, db_req->export_timer) >= db_req->export_period)
	{
		log_db_export_data(db_req->db, in_time);
		log_db_clean_old_data(db_req->db, in_time, db_req->storage_deep);

		//  update timer
		db_req->export_timer = in_time;
	}
}


void log_db_maintain_databases(time_t in_time)
{
	if(db_sync_timer > 0 && difftime(in_time, db_sync_timer) >= db_sync_period)
		log_db_file_sync();

	log_db_maintain_database(&load_profile_db, in_time);
	log_db_maintain_database(&consum_arch_db, in_time);
	log_db_maintain_database(&dev_event_db, in_time);
	log_db_maintain_database(&app_event_db, in_time);
}


int log_db_get_var(log_db *db_req, uint32_t adr, char *var_name, int len, time_t *log_time, uint32_t *value)
{
	DBT key, data;
	DBC *cursor = NULL;
	time_t rec_time;
	int ret, idx;

	if(!db_env || !db_req || !db_req->db || db_req->flds_num == 0 || !var_name || len == 0 || !log_time || !value) return -1;

	idx = log_db_get_fld_idx(db_req, var_name);

	if(idx < 0) return -1;

    ret = db_req->db->cursor(db_req->db, NULL, &cursor, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cursor setup failed for %s DB: %s (%d)\n", db_req->db->dname, db_strerror(ret), ret);

	    return -1;
	}

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

	pthread_mutex_lock(&db_log_mtx);

	// go through log records in the loop
	while((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0)
    {
		rec_time = ntohl( *((uint32_t*)key.data) );

    	if(rec_time >= *log_time) break;
    }

	if(ret == 0)
	{
		*log_time = rec_time;
		*value = ntohl( *((uint32_t*)data.data + idx) );
	}
	else
	{
		ret = -1;
	}

	pthread_mutex_unlock(&db_log_mtx);

	cursor->c_close(cursor);
	cursor = NULL;

	return ret;
}


int log_db_get_event(log_db *db_req, uint32_t adr, time_t *log_time, char **msg, int *len)
{
	DBT key, data;
	DBC *cursor = NULL;
	uint32_t *rec_adr;
	int ret;

	if(!db_env || !db_req || !db_req->db || db_req->flds_num != 0 || !log_time || !msg || !len) return -1;

    ret = db_req->db->cursor(db_req->db, NULL, &cursor, 0);

	if(ret != 0)
	{
	    ts_printf(STDOUT_FILENO, "IEC61850: BDB - Cursor setup failed for %s DB: %s (%d)\n", db_req->db->dname, db_strerror(ret), ret);

	    return -1;
	}

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

	pthread_mutex_lock(&db_log_mtx);

	// go through log records in the loop
	while((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0)
    {
		*log_time = ntohl( *((uint32_t*)key.data) );

		rec_adr = (uint32_t*)data.data;

		if(*rec_adr == adr) break;
    }

	if(ret == 0)
	{
		*msg = (char*) malloc(data.size - sizeof(uint32_t));
		memcpy((void*) *msg, (void*) ((char*)data.data+sizeof(uint32_t)), data.size - sizeof(uint32_t));
		*len = data.size - sizeof(uint32_t);
	}
	else
	{
		ret = -1;
	}

	pthread_mutex_unlock(&db_log_mtx);

	cursor->c_close(cursor);
	cursor = NULL;

	return ret;
}
