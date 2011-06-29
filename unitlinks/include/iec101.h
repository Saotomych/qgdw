/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC101_H_
#define _IEC101_H_


#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include "p_num.h"
#include "iec_def.h"
#include "lpdu_frame.h"
#include "../../common/asdu.h"
#include "iec_asdu.h"
#include "../../devlinks/devlink.h"
#include "../../common/multififo.h"



#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */
#define APP_NAME	"unitlink-iec101"
#define APP_PATH 	"/rw/mx00/unitlinks"

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




#ifdef __cplusplus
}
#endif

#endif //_IEC101_H_
