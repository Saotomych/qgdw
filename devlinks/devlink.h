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

struct iec_data_type{
	char	*iec_name;
};

struct work_data_type{
	u32	val;
	u16 priority;	// priority to send
	u16 eventsubs;	// subscribe to change data
};

