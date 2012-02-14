/*
 * varcontrol.h
 *
 *  Created on: 27.01.2012
 *      Author: alex
 */

#ifndef VARCONTROL_H_
#define VARCONTROL_H_

#include "iec61850.h"

#define BOOKING		0x100
#define TRUEVALUE	0x080
#define ISBOOKED	0x040
#define INTVAR		0x020
#define INTCONST	0x010

// Types
#define STRING		1
#define INT32		2
#define INT64		4
#define PTRSTRING	8
#define PTRINT32	0x10
#define PTRINT64	0x20
#define INT32DIG2	0x40

// IEC struct
typedef struct _FCDAREC{
	// Name of variable as attributes FCDA
	char *fc;		// Full name of variable is <LD.name>/<LN.prefix>.<LN.class><LN.inst>.<DO.name>.<DA.name> - required
	char *ldinst;	// we has as <LD.name> now - optional
	char *prefix;	// LN.prefix - optional
	char *lnClass;	// LN.class - optional
	char *lnInst;	// LN.inst - optional
	char *doName;	// DO.name - optional
	char *daName;	// DA.name - optional
} fcdarec, *pfcdarec;

typedef struct _VALUE{
	char *name;		// full name of variable
	void *val;		// pointer to value
	void *defval;	// pointer to default value (if exists), using if val == NULL or value not true
	int idtype;		// C type of variable as enum
	int iddeftype;	// C type of default value may be other
} value, *pvalue;

// My working struct for connecting IEC struct with real value
typedef struct _VARREC{
	LIST l;
	fcdarec *name;	// Can see as pointer to pointer to full name already
	value *val;		// Values and types of variable
	int prop;		// properties: const, var, booking, true value.
	int time;		// time from last refresh value, usec
} varrec, *pvarrec;

// Structure for fast working events while variable was changed
typedef struct _VARBOOK{
	LIST l;
	int idvar;		// it's pointer to var in other application. = val
	int idtype;		// it's var type
	void *val;
	int	prop;
} varbook, *pvarbook;

extern void vc_init(pvalue vt, int len);
//extern varrec *vc_addvarrec(char *varname, LNODE *actln);
extern varrec *vc_addvarrec(char *varname, varrec *actvr);
extern int vc_rmvarrec(char *varname);

#endif /* VARCONTROL_H_ */
