/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _ICDXML_H_
#define _ICDXML_H_


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

#define SCADA_NAME		"scada"
#define SCADA_DESC		"Center"
#define METER_NAME		"m700.place001"
#define METER_DESC		"Gallery, bay1"




/*
 *
 * Structures
 *
 */

typedef struct _DOBJ{
	char *name;
	uint32_t asdu_pos;
} dobj;


/*
 *
 * Functions
 *
 */

void icd_full(char *fname);
void icd_begin(FILE *fdesc, char *id, char *name);
void icd_substation(FILE *fdesc, char *name, char *desc);
void icd_ied_scada(FILE *fdesc, char *name, char *desc);
void icd_ied_meter(FILE *fdesc, char *name, char *desc);
void icd_dtype_tmpl(FILE *fdesc);
void icd_lnode_type(FILE *fdesc);
void icd_do_type(FILE *fdesc);
void icd_end(FILE *fdesc);

void icd_map_read(const char *file_name);

#ifdef __cplusplus
}
#endif

#endif //_ICDXML_H_
