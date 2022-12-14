/*
 * iec61850.h
 *
 *  Created on: 13.07.2011
 *      Author: alex
 */

#ifndef IEC61850_H_
#define IEC61850_H_

#define IECCONFIG		"ieclevel.cid"

#define ALARM_PER		2  // must be >= 2 sec!!!

struct _BATTR;
struct _BATTR_LIST;
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
struct _LDEVICE;
struct _LDEVICE_LIST;
struct _IED;
struct _IED_LIST;

extern u08 Encoding;

/* Base Attributes */
typedef struct _BATTR_LIST{
	LIST l;
	struct _BATTR{
		char *name;
		char *btype;
		struct _ATYPE *pmyattrtype;		// xml-parser set up this value
		int  mytype;
	} battr;
} BATTR;

/* Attributes type */
typedef struct _ATYPE_LIST{
	LIST l;
	struct _ATYPE{
		char *id;
		BATTR *pfbattr;						// crossconnector set up this value
		int maxbattr;						// xml-parser set up this value
	} atype;
} ATYPE;

/* Attributes */
typedef struct _ATTR_LIST{
	LIST l;
	struct _ATTR{
		char *name;
		char *btype;
		char *dchg;
		char *qchg;
		char *dupd;
		char *fc;
		char *type;
		struct _DTYPE *pmydatatype;		// xml-parser set up this value
		struct _ATYPE *pmyattrtype;		// xml-parser set up this value
		int  mytype;
	} attr;
} ATTR;

/* Data type */
typedef struct _DTYPE_LIST{
	LIST l;
	struct _DTYPE{
		char *id;
		char *cdc;
		ATTR *pfattr;						// crossconnector set up this value
		int maxattr;						// xml-parser set up this value
	} dtype;
} DTYPE;

/* Data object */
typedef struct _DO_LIST{
	LIST l;
	struct _DOBJ{
		char *name;
		char *type;
		char *options;
		struct _DTYPE	*pmytype;			// crossconnector set up this value
		struct _LNODETYPE *pmynodetype;		// xml-parser set up this value
	} dobj;
} DOBJ;

/* Logical Node Type */
typedef struct _LNODETYPE_LIST{
	LIST l;
	struct _LNODETYPE{
		char *id;
		char *lnclass;
		DOBJ *pfdobj;		// crossconnector set up this value
		int  maxdobj;		// xml-parser set up this value
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
		char *prefix;
		struct _IED *pmyied;			// crossconnector set up this value
		struct _LDEVICE *pmyld;			// crossconnector set up this value
		struct _LNODETYPE *pmytype;		// crossconnector set up this value
	} ln;
} LNODE;

/* Logical Device */
typedef struct _LDEVICE_LIST{
	LIST l;
	struct _LDEVICE{
		char *inst;
		char *desc;
		struct _IED *pmyied;			// crossconnector set up this value
	} ld;
} LDEVICE;

/* Intellectual Electronic Device */
typedef struct _IED_LIST{
	LIST l;
	struct _IED{
		char *name;
		char *desc;
	} ied;
} IED;

// Real Attributes for every DOBJ
typedef struct _iec_attr_unit{
	LIST l;
	union {
		uint32_t	ui;					/* unsigned integer representation */
		int32_t		i;					/* integer representation */
		float		f;					/* float representation */
	} attr;							/* transferring value (e.g. measured value, counter readings, etc.) */
	ATTR *myattr;
} IEC_ATTR_UNIT;

// Real Values for every LN
typedef struct _iec_data_unit{
	LIST l;
	union {
		uint32_t	ui;					/* unsigned integer representation */
		int32_t		i;					/* integer representation */
		float		f;					/* float representation */
	} value;							/* transferring value (e.g. measured value, counter readings, etc.) */
	DOBJ *mydobj;
} IEC_DATA_UNIT;

extern LIST fied, fld, fln, flntype, fdo, fdtype, fattr;

extern int cid_build(char *filename);
extern int virt_start(char *appname);
extern void crossconnection(void);
extern void catch_alarm(int sig);

extern LIST* create_next_struct_in_list(LIST *plist, int size);	// call create new LIST item

extern void cid_create_ied(const char *pTag);			// call parse ied
extern void cid_create_ld(const char *pTag);			// call parse ld
extern void cid_create_ln(const char *pTag);			// call parse ln
extern void cid_create_lntype(const char *pTag);		// call parse lntype
extern void cid_create_dobj(const char *pTag);			// call parse do
extern void cid_create_dobjtype(const char *pTag);		// call parse dotype
extern void cid_create_attr(const char *pTag);			// call parse attr
extern void cid_create_attrtype(const char *pTag);		// call parse attrtype
extern void cid_create_bda(const char *pTag);			// call parse bda
extern void cid_create_enum(const char *pTag);			// call parse enumtype
extern void cid_create_enumval(const char *pTag);		// call parse enumval
extern void cid_create_subst(const char *pTag);			// call parse substation


#endif /* IEC61850_H_ */
