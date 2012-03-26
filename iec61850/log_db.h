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

/*
 *
 * Structures
 *
 */

typedef struct log_db log_db;

struct log_db {
	DB		*db;
	char	**flds;
	int		flds_num;
	time_t	add_period;
	time_t	add_timer;
	time_t	export_period;
	time_t	export_timer;
	time_t	storage_deep;
	void	(*add_var_rec)(log_db *db_req, uint32_t adr, uint32_t *log_rec);
	int		(*get_var)(log_db *db_req, uint32_t adr, char *var_name, int len, time_t *log_time, uint32_t *value);
	void	(*add_event)(log_db *db_req, uint32_t adr, char *msg, int len);
};


/*
 *
 * Functions
 *
 */
int log_db_env_open();
void log_db_env_close();

void log_db_init_timers(time_t init_time);

void log_db_maintain_databases(time_t cut_time);


#ifdef __cplusplus
}
#endif

#endif //_LOG_DB_H_
