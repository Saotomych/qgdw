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


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

#define LOG_FLD_NUM			25
#define LOG_LP_FLD_NUM		25
#define LOG_CA_FLD_NUM		4


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
int log_db_env_open();
void log_db_env_close();


int log_db_get_fld_idx(char *var_name);

void log_db_load_profile_add_rec(uint32_t adr, uint32_t *log_rec);
void log_db_consum_arch_add_rec(uint32_t adr, uint32_t *log_rec);



#ifdef __cplusplus
}
#endif

#endif //_LOG_DB_H_
