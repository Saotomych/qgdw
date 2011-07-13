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
