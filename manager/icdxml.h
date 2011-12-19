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
#define IED_NAME		"TEMPLATE"




/*
 *
 * Structures
 *
 */

typedef struct _LNODETYPE{
	char		*id;
	char		*class;
	char		*prefix;
} lntype;



typedef struct _DOBJ{
	char		*lntypeid;
	char		*name;
} dobj;


/*
 *
 * Functions
 *
 */

void icd_full(char *fname);
void icd_begin(FILE *fdesc, char *id, char *name);
void icd_substation(FILE *fdesc, char *name, char *desc);
void icd_ied(FILE *fdesc, char *name, char *desc);
void icd_ied_scada(FILE *fdesc, char *name, char *desc);
void icd_ied_meter(FILE *fdesc, char *name, char *desc);
void icd_dtype_tmpl(FILE *fdesc);
void icd_lnode_type(FILE *fdesc);
void icd_do_type(FILE *fdesc);
void icd_end(FILE *fdesc);

#ifdef __cplusplus
}
#endif

#endif //_ICDXML_H_
