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

/* XML Params */
#define WIN	1
#define DOS 2
#define UTF 3
extern u08 Encoding;

/* Just List */
typedef struct _LIST{
	void *next;
	void *prev;
} LIST;

/* Attributes */
typedef struct _ATTR_LIST{
	LIST l;
	struct _ATTR{
		char *name;
		char *btype;
		char *dchg;
		char *fc;
		struct _DTYPE *pmydatatype;
		int  mytype;
	} attr;
} ATTR;

/* Data type */
typedef struct _DTYPE_LIST{
	LIST l;
	struct _DTYPE{
		char *id;
		char *cdc;
		ATTR *pfattr;
	} dtype;
} DTYPE;

/* Data object */
typedef struct _DO_LIST{
	LIST l;
	struct _DOBJ{
		char *name;
		char *type;
		char *options;
		struct _DTYPE *pmydatatype;
		struct _LNODETYPE *pmynodetype;
	} dobj;
} DOBJ;

/* Logical Node Type */
typedef struct _LNODETYPE_LIST{
	LIST l;
	struct _LNODETYPE{
		char *id;
		char *lnclass;
		DOBJ *pfdobj;
		int  maxdobj;
	} lntype;
} LNTYPE;

/* Logical Node */
typedef struct _LNODE_LIST{
	LIST l;
	struct _LNODE{
		char *lninst;
		char *lnclass;
		char *lntype;
		char *iedname;
		char *ldinst;
		struct _IED *pmyied;
		struct _LNODETYPE *pmytype;
	} ln;
} LNODE;

/* Intellectual Electronic Device */
typedef struct _IED_LIST{
	LIST l;
	struct _IED{
		char *name;
		char *inst;
		LNODE *fln;
	} ied;
} IED;

extern int virt_start();

extern void ssd_create_ied(const char *pTag);			// call parse ied
extern void ssd_create_ln(const char *pTag);			// call parse ln
extern void ssd_create_lntype(const char *pTag);		// call parse lntype
extern void ssd_create_dobj(const char *pTag);			// call parse do
extern void ssd_create_dobjtype(const char *pTag);		// call parse dotype
extern void ssd_create_attr(const char *pTag);			// call parse attr
extern void ssd_create_enum(const char *pTag);			// call parse enumtype
extern void ssd_create_enumval(const char *pTag);		// call parse enumval
extern void ssd_create_subst(const char *pTag);		// call parse substation


#endif /* IEC61850_H_ */
