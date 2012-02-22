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
	char	*name;
} tarif;

typedef struct _sett{
	LIST l;
	u_char	id;
	u_char	hour;
	u_char	min;
	u_char	idtarif;
	tarif	*mytarif;
} sett;

typedef struct _season{
	LIST 	l;
	LIST 	mywrdays;		// Set of switches of work day for this season
	LIST 	myhldays;      // Set of switches of holiday for this season
	int 	id;
	char 	*name;
	u_char	mnth;
	u_char	day;
} season;

typedef struct _specday{
	LIST l;
	int		daytype;
	int		id;
	LIST	mysett;
	u_char	mnth;
	u_char	day;
} specday;

typedef struct _highday{
	LIST l;
	LIST myhgdays;
} highday;

extern LIST fseason, fspecs, ftarif, fhighday;

extern int tarif_parser(char *filename);
extern void connect_2tarif(void);
extern void tarifvars_init(const char *pTag);
extern void create_season(const char *pTag);
extern void start_workdaysset(const char *pTag);
extern void start_holidaysset(const char *pTag);
extern void start_highdaysset(const char *pTag);
extern void add_workday(const char *pTag);
extern void add_holiday(const char *pTag);
extern void add_highday(const char *pTag);
extern void create_set(const char *pTag);
extern void create_tarif(const char *pTag);

#endif /* TARIF_H_ */
