/*
 * tarif.h
 *
 *  Created on: 21.02.2012
 *      Author: alex
 */

#ifndef TARIF_H_
#define TARIF_H_

// Day types
#define	WORKDAY		10
#define HOLIDAY		11
#define HIGHDAY		12

typedef struct _tarif{
	LIST l;
	int 	id;
	float	money;
	char	*desc;
} tarif;

typedef struct _sett{
	LIST l;
	u_char	id;
	u_char	hour;
	u_char	min;
	u_char	tarif;
	tarif	*mytarif;
} sett;

typedef struct _season{
	LIST l;
	sett 	*mywrdays;		// Set of switches of work day for this season
	sett 	*myhldays;      // Set of switches of holiday for this season
	int 	id;
	char 	*name;
	u_char	mnth;
	u_char	day;
} season;

typedef struct _specday{
	LIST l;
	int		daytype;
	int		id;
	sett	*mysett;
	u_char	mnth;
	u_char	day;
} specday;

extern sett fhighdays;

extern LIST fseason, fspecs, ftarif, fmyhgdays;

#endif /* TARIF_H_ */
