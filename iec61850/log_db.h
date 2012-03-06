/*
 * Copyright (C) 2012 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _LOG_DB_H_
#define _LOG_DB_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <db.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

#define LOG_FLD_NUM			25
#define LOG_SERV_PRM_SIZE	2


/*
 *
 * Structures
 *
 */

typedef struct log_db_serv_prm {
	char			*db_name;
	DB				*db_req;
	char			**db_flds;
	int				flds_num;
	const time_t	add_period;
	time_t			add_timer;
	const time_t	export_period;
	time_t			export_timer;
	const time_t	storage_deep;
} log_db_serv_prm;

extern log_db_serv_prm log_serv_prm_list[LOG_SERV_PRM_SIZE];


/*
 *
 * Functions
 *
 */
int log_db_env_open();
void log_db_env_close();

void log_db_init_timers();

int log_db_get_fld_idx(char *var_name);

void log_db_add_var_rec(uint32_t adr, uint32_t *log_rec, DB *db_req, char **log_flds, int fld_num);

int log_db_export_data(DB *db_req, time_t cut_time, char *db_file_out);

int log_db_clean_old_data(DB *db_req, time_t cut_time, time_t storage_deep);




#ifdef __cplusplus
}
#endif

#endif //_LOG_DB_H_
