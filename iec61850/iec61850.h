/*
 * iec61850.h
 *
 *  Created on: 13.07.2011
 *      Author: alex
 */

#ifndef IEC61850_H_
#define IEC61850_H_

struct _ATTR;
struct _ATTR_LIST;
struct _DTYPE;
struct _DTYPE_LIST;
struct _DOBJ;
struct _DO_LIST;
struct _LNODETYPE;
struct _LNODETYPE_LIST;
struct _LNODE;
struct _LNODE_LIST;
struct _IED;
struct _IED_LIST;

/* Attributes */
typedef struct _ATTR_LIST{
	struct _ATTR{
		char *name;
		char *btype;
		char *dchg;
		char *fc;
		struct _DTYPE *pmydatatype;
		int  mytype;
	} attr;
	struct _ATTR_LIST	*next;
	struct _ATTR_LIST	*prev;
} ATTR;

/* Data type */
typedef struct _DTYPE_LIST{
	struct _DTYPE{
		char *id;
		char *cdc;
		ATTR *pfattr;
	} dtype;
	struct _DTYPE_LIST	*next;
	struct _DTYPE_LIST	*prev;
} DTYPE;

/* Data object */
typedef struct _DO_LIST{
	struct _DOBJ{
		char *name;
		char *type;
		char *options;
		struct _DTYPE *pmydatatype;
		struct _LNODETYPE *pmynodetype;
	} dobj;
	struct _DO_LIST		*next;
	struct _DO_LIST		*prev;
} DOBJ;

/* Logical Node Type */
typedef struct _LNODETYPE_LIST{
	struct _LNODETYPE{
		char *id;
		char *lnclass;
		DOBJ *pfdobj;
		int  maxdobj;
	} lntype;
	struct _LNODETYPE_LIST *next;
	struct _LNODETYPE_LIST *prev;
} LNTYPE;

/* Logical Node */
typedef struct _LNODE_LIST{
	struct _LNODE{
		char *lninst;
		char *lnclass;
		char *lntype;
		struct _IED *pmyied;
		struct _LNODETYPE *pmytype;
	} ln;
	struct _LNODE_LIST	*next;
	struct _LNODE_LIST	*prev;
} LNODE;

/* Intellectual Electronic Device */
typedef struct _IED_LIST{
	struct _IED{
		char *name;
		char *inst;
		LNODE *fln;
	} ied;
	struct _IED_LIST *next;
	struct _IED_LIST *prev;
} IED;


extern int ssd_create_ied(char* cstr);			// call parse ied
extern int ssd_create_ln(char* cstr);				// call parse ln
extern int ssd_create_data(char* cstr);			// call parse data
extern int ssd_create_datatype(char* cstr);		// call parse data_type
extern int ssd_create_enum(char* cstr);			// call parse enum
extern int ssd_create_subst(char* cstr);			// call parse substation

#endif /* IEC61850_H_ */
