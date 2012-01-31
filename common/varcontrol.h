/*
 * varcontrol.h
 *
 *  Created on: 27.01.2012
 *      Author: alex
 */

#ifndef VARCONTROL_H_
#define VARCONTROL_H_

#define BOOKING		0x100
#define TRUEVALUE	0x080
#define ISBOOKED	0x040

typedef struct _VARTABLE{
	char syn[20];	// 'Synonym' of variable
	void *val;		// pointer to value
	void *defval;	// pointer to default value (if exists)
	int idtype;		// type of variable
	int prop;		// properties: booking, true value.
	int time;		// time from last refresh value, sec
} vartable, *pvartable;

typedef struct _VARBOOK{
	int idvar;		// it's pointer to var in other application. = val
	int idtype;		// it's var type
	void *val;
	int	prop;
} varbook, *pvarbook;

#endif /* VARCONTROL_H_ */
