// Data descriptions for data conversion one real device into data_types IEC61850

#include "../common/types.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_DEVICELINK	32

// Config device struct
struct config_device{
	char   	*name;
	char 	*protoname;			// ptr to protokol name
	char 	*phyname;			// physlink number
	u32		addr;
	u32		extaddr;
};

struct iec_data_type{
	char	*iec_name;
};

struct work_data_type{
	u32	val;
	u16 priority;	// priority to send
	u16 eventsubs;	// subscribe to change data
};

struct data_type{
	void *next;		// pointer to next data_type
	struct iec_data_type *iec_dt;
	// Work Attributes
	struct work_data_type *work_dt;
	u32 numasdu;
	u32 index;		// position inside ASDU
	u16 priority;	// priority to send
	u16 eventsubs;	// subscribe to change data
};

// In mind !!!
//// Открытые дескрипторы к парсерам: тип, перечень
//struct hnd_parser{
//	struct hnd_parser *next;
//	char *protoname;			// ptr to protokol name
//	char *filepath;
//	FILE *f_dev2parser[2];		// fifo descriptor
//	char *f_names[2];			// fifo names
//};
//
//// Dev link struct
//struct device_link{
//	u32 sysnum;
//	u32 devtype;
//	struct data_type *first_dt;			// формируют связанный список для кольцевого опроса
//	struct work_data_type *first_wdt;	// формируют связанный список служебных переменных
//	struct config_device *devconf;
//	struct hnd_parser *hpars;				// Handle for pipe to parser
//	int (*proloque)(char *buf, int len, char *newbuf);		// return packet's len
//	int (*epiloque)(struct data_type *dt);
//};
//
//// Protokol -> proloque + epiloque
//struct prolepi{
//	char *protoname;
//	void *funcproloq;
//	void *funcepiloq;
//};
