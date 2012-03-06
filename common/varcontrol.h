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
#define INTERNAL	0x020
#define NEEDFREE	0x010

// Types
#define STRING		1
#define INT32		2
#define INT64		4
#define PTRSTRING	8
#define PTRINT32	0x10
#define PTRINT64	0x20
#define INT32DIG2	0x40
#define FLOAT32		0x80
#define AVALUE		0x100
#define QUALITY		0x200
#define TIMESTAMP	0x400

// IEC Types for varrec
#define IECBASE		0x1000
#define IEDdesc			1
#define IEDname			2
#define LDinst			3
#define LDname			4
#define LDdesc			5
#define LNlnclass		6
#define LNlninst		7
#define LNlntype		8
#define LNprefix		9
#define DOdesc			10
#define DOname			11
#define DOtype			12
#define DOvalue			13
#define DAname			14
#define DAbtype			15
#define DAtype			16
#define DAfc			17
#define DAdchg			18
#define DAdupd			19
#define DAqchg			20

#define IECVALUE		1000

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
	int	idx;		// Index of value in defvalues
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
	int iarg;
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
//extern varrec *vc_addvarrec(LNODE *actln, char *varname, varrec *actvr, value *defvr);
extern varrec *vc_addvarrec(LNODE *actln, char *varname, value *defvr);
extern int vc_destroyvarreclist(varrec *fvr);

#endif /* VARCONTROL_H_ */