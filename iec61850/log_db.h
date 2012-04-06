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
#include "../common/common.h"


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
typedef struct log_event {
	uint32_t	event_adr;
	time_t		event_time;
	char		*event_msg;
} log_event;


typedef struct log_db log_db;

struct log_db {
	DB		*db;
	char	*flds_list;
	int		flds_num;
	time_t	add_period;
	time_t	add_timer;
	time_t	export_period;
	time_t	export_timer;
	time_t	storage_deep;
	void	(*add_var_rec)(log_db *db_req, uint32_t adr, uint32_t *log_rec);
	int		(*get_vars)(log_db *db_req, uint32_t adr, char *var_name, time_t log_time, time_t intr, int num, varevent *vars);
	void	(*add_event)(log_db *db_req, uint32_t adr, char *msg, int len);
	int		(*get_events)(log_db *db_req, uint32_t adr, time_t st_time, time_t end_time, log_event **events);
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
