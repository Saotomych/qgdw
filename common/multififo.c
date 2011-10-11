/*
 * multififo.c
 *
 *  Created on: 09.06.2011
 *      Author: Alex AVAlon
 *
 * Control connections to/from applications by pair fifos
 *
 */

#include "../common/common.h"
#include "../common/asdu.h"
#include "multififo.h"

#define LENRINGBUF	1024
#define INOTIFYTHR_STACKSIZE	64*1024

char *appname, *pathapp;

// Control child
volatile static int inotifystop = 0;
int hpp[2];

// Control inotify
static int d_inoty = 0;

// Control channel
char *sufinit = {"-init"};
char *sufdn = {"-dn"};
char *sufup = {"-up"};

int (*cb_rcvdata)(int len);

struct channel{
	char *appname;
	char *f_namein;
	char *f_nameout;
	int descin;
	int descout;
	int mode;
	int events;
	int watch;
	int (*in_open)(struct channel *ch);		// IN_OPEN
	int (*in_close)(struct channel *ch);	// IN_CLOSE
	int (*in_read)(struct channel *ch);		// IN_MODIFY
	char ring[LENRINGBUF];
	char *bgnframe;	// start receiving frame
	char *bgnring;	// start data not readed since frame start
	char *endring;	// end frame
};

// connect device to channel
struct endpoint{
	ep_init_header			eih;
	struct channel			*cdcup;
	struct channel			*cdcdn;
	int my_ep;			// unique endpoint's identify application inside
	int ep_up;			// uplevel endpoint's number
	int ep_dn;			// downlevel endpoint's number
	volatile u08 ready;
};

// List of channels
static int maxch = 0;
static struct channel *mychs[MAXCH];
static struct channel *actchannel;	// Actual channel for data reading

// List of endpoints
static int maxep = 1;
static struct endpoint *myeps[MAXEP];

// ================= Callbacks PROTOTYPES ================================== //
int init_open(struct channel *ch);
int sys_open(struct channel *ch);
int init_close(struct channel *ch);
int sys_close(struct channel *ch);
int init_read(struct channel *ch);
int sys_read(struct channel *ch);

// =================== Private functions =================================== //

// Read from fifo to ring buffer
// Return not read lenght from ch->bgnring to ch->endring
int readchannel(struct channel *ch){
char *endring = ch->ring + LENRINGBUF;
int tail;
int rdlen=0, len;
int notreadlen;

	if ((ch->bgnframe == ch->endring) && (ch->bgnring != ch->endring)){
		ch->events &= IN_MODIFY;	// Buffer overflow, read off
		return -2;
	}

	if (ch->bgnring > ch->endring){
		tail = ch->bgnring - ch->endring;
		notreadlen = ch->endring + LENRINGBUF - ch->bgnring ;
	}else{
		tail = endring - ch->endring;
		notreadlen = ch->endring - ch->bgnring;
	}

	rdlen = read(ch->descin, ch->endring, tail);
	if (rdlen == -1) return -1;

	ch->endring += rdlen;
	if (rdlen == tail){
		len = read(ch->descin, ch->ring, ch->bgnframe - ch->ring);
		if (len != -1){
			ch->endring = ch->ring + len;
			rdlen += len;
		}
	}

	return rdlen + notreadlen;
}

// Move string to user bufferBuilding endpoint
// Frame don't take from ring buffer
int getstringfromring(struct channel *ch, char *buf){
char *p, *pbuf;
char *endring = ch->ring + LENRINGBUF;
int l = 0;
	p = ch->bgnring;
	pbuf = buf;
	while((*p) && (p != ch->endring)){
		*pbuf = *p;
		pbuf++; p++; l++;
		if (p >= endring) p = ch->ring;
	}
	if (p == ch->endring) return -1;
	*pbuf=0; l++;	// include ending zero
	ch->bgnring = p+1;
	return l;
}

int getdatafromring(struct channel *ch, char *buf, int len){
int len1, len2;
char *endring = ch->ring + LENRINGBUF;
char *end = ch->bgnring + len;
	if (len > LENRINGBUF) return -1;
	if (end < endring){
		len1 = len;
		len2 = 0;
		if (ch->endring < end) return -1;
	}else{
		len1 = endring - ch->bgnring;
		len2 = len - len1;
		if (ch->endring < (ch->ring+len2)) return -1;
		memcpy(&buf[len1], ch->ring, len2);
	}
	memcpy(buf, ch->bgnring, len1);

	// Move pointer
	if (len2) ch->bgnring = ch->ring + len2;
	else ch->bgnring += len1;

	return len;
}

int getframefromring(struct channel *ch, char *buf, int len){
int len1, len2;
char *endring = ch->ring + LENRINGBUF;
char *end = ch->bgnframe + len;
	if (len > LENRINGBUF) return -1;
	if (end < endring){
		len1 = len;
		len2 = 0;
		if (ch->endring < end) return -1;
	}else{
		len1 = endring - ch->bgnframe;
		len2 = len - len1;
		if (ch->endring < (ch->ring+len2)) return -1;
		memcpy(&buf[len1], ch->ring, len2);
	}
	memcpy(buf, ch->bgnframe, len1);

	// Move pointers
	if (len2) ch->bgnframe = ch->ring + len2;
	else ch->bgnframe += len1;
	ch->bgnring = ch->bgnframe;

	return len;
}

int getframefalse(struct channel *ch){
	ch->bgnframe = ch->bgnring;
	return 0;
}

int testrunningapp(char *name){
char *par[3];
char *env[1];
char pidof[] = {"/bin/pidof"};
char buf[160] = {0};
int ret, pid, wait_st;
int opipe[2];
struct timeval tv;
fd_set readset;

		par[0] = pidof;
		par[1] = name;
		par[2] = NULL;
		env[0] = NULL;

		if (pipe(opipe) == -1)
		{
			printf("MFI %s: failed to create pipe", appname);
			return -1;
		}
		ret = fork();
		if (ret < 0){
			// fork failed
			printf("MFI %s: fork failed: %d - %s\n", appname, errno, strerror(errno));
			return -1;
		}
		if (!ret){
			// child process
	        close(1);
	        dup2(opipe[1], 1);
	        close(opipe[0]);
	        close(opipe[1]);
			execv(pidof, par);
			printf("MFI %s: testrunnigapp failed: %d - %s\n",appname, errno, strerror(errno));
			exit(0);
		}
		pid = ret;
		waitpid(pid, &wait_st, 0);
//		FD_ZERO(&readset);
	    FD_SET(opipe[0], &readset);
	    close(opipe[1]);
	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(opipe[0] + 1, &readset, 0, 0, &tv);
	    if (ret > 0){
	    	ret = read(opipe[0], buf, 160);
	    	return ret;
	    }
		waitpid(pid, &wait_st, 0);
	    return -1;
}

struct endpoint *find_ep_by_addr(u32 addr){
int i;
	for(i = 1; i < maxep; i++){
		if (myeps[i]->eih.addr == addr) return myeps[i];
	}
	return NULL;
}

struct channel *findch_by_name(char *nm){
int i;
	if (maxch < 2) return 0;
	for(i=1; i < maxch; i++){

		if (!strcmp(nm, mychs[i]->appname)) break;
	}
	return (i == maxch ? 0 : mychs[i]);
}

// =================== End Private functions ================================= //

// ======================= Init functions ==================================== //

void initchannelstruct(struct channel *ch){
	ch->descin = 0;
	ch->descout = 0;
	ch->mode = S_IFIFO | S_IRUSR  | S_IWUSR;
	ch->events = IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_MODIFY;
	ch->watch = 0;
	ch->bgnring = ch->ring;
	ch->bgnframe = ch->ring;
	ch->endring = ch->ring;
}

// Create init channel, it have index = 0 always
// High level application opens init channel for sending struct ep_init_header => register new endpoint
struct channel *initchannel(char *a_path, char *a_name){
int len;
struct channel *ch=0;

	if (mychs[0]) return mychs[0];

	mychs[0] = malloc(sizeof(struct channel));
	ch = mychs[0];
	if (!ch) return 0;
	initchannelstruct(ch);

	ch->appname = malloc(strlen(a_name));
	strcpy(ch->appname, a_name);

	len = strlen(a_path) + strlen(a_name) + strlen(sufinit) + 8;
	ch->f_namein = malloc(len);
	sprintf(ch->f_namein, "%s/%s%s", a_path,a_name,sufinit);

	ch->f_nameout = 0;

	ch->in_open = init_open;
	ch->in_close = init_close;
	ch->in_read = init_read;

	unlink(ch->f_namein);
	if (mknod(ch->f_namein, ch->mode, 0)) return 0;

	maxch++;

//	printf("MFI %s: INIT CHANNEL %d READY, MAXCH = %d\n", appname, maxch-1, maxch);
//	printf("MFI %s: init channel file ready: %s\n", appname, mychs[maxch-1]->f_namein);

	return ch;
}

// Create new two-direction channel to downlink
// Channel works for data exchange with lower application
struct channel *newchanneldn(char *a_path, char *a_name){
int len;
struct channel *ch=0;
	// Create downdirection FIFO

	mychs[maxch] = malloc(sizeof(struct channel));
	ch = mychs[maxch];
	if (!ch) return 0;
	initchannelstruct(ch);

	ch->appname = malloc(strlen(a_name) + 1);

	strcpy(ch->appname, a_name);

	len = strlen(a_path) + strlen(a_name) + strlen(sufup) + 3;
	ch->f_namein = malloc(len);
	sprintf(ch->f_namein, "%s/%s%s", a_path, a_name, sufup);

	len = strlen(a_path) + strlen(a_name) + strlen(sufdn) + 3;
	ch->f_nameout = malloc(len);
	sprintf(ch->f_nameout, "%s/%s%s", a_path, a_name, sufdn);

	unlink(ch->f_namein);
	if (mknod(ch->f_namein, ch->mode, 0)) return 0;
	unlink(ch->f_nameout);
	if (mknod(ch->f_nameout, ch->mode, 0)) return 0;

	ch->in_open = sys_open;
	ch->in_close = sys_close;
	ch->in_read = sys_read;

	ch->watch = inotify_add_watch(d_inoty, ch->f_namein, ch->events);

	maxch++;

//	printf("MFI %s: initialize down channel %d ready, MAXCH = %d\n", appname, maxch-1, maxch);
//	printf("MFI %s: CHANNEL FILES READY: in - %s & out - %s\n", appname, mychs[maxch-1]->f_namein, mychs[maxch-1]->f_nameout);

	return ch;
}

// Create new two-direction channel to uplink
// Connect to fifos from high level application
// Channel work for data exchange with higher application
struct channel *newchannelup(char *a_path, char *a_name, char *a_name_up){
int len;
struct channel *ch=0;
	// Create updirection FIFO

	mychs[maxch] = malloc(sizeof(struct channel));
	ch = mychs[maxch];
	if (!ch) return 0;

	initchannelstruct(ch);

	ch->appname = malloc(strlen(a_name_up) + 1);
	strcpy(ch->appname, a_name_up);

	len = strlen(a_path) + strlen(a_name) + strlen(sufdn) + 3;
	ch->f_namein = malloc(len);
	sprintf(ch->f_namein, "%s/%s%s", a_path, a_name, sufdn);

	len = strlen(a_path) + strlen(a_name) + strlen(sufup) + 3;
	ch->f_nameout = malloc(len);
	sprintf(ch->f_nameout, "%s/%s%s", a_path, a_name, sufup);

	ch->in_open = sys_open;
	ch->in_close = sys_close;
	ch->in_read = sys_read;

	maxch++;

//	printf("MFI %s: CHANNEL FILES READY: in - %s & out - %s\n", appname, ch->f_namein, ch->f_nameout);

	return ch;
}

struct endpoint *create_ep(u32 addr){
struct endpoint *ep=0;
	if (maxep > 63) return NULL;
	ep = malloc(sizeof(struct endpoint));
	if(!ep) return NULL;

	myeps[maxep] = ep;
	ep->my_ep = maxep;

	ep->eih.addr = addr;
	ep->eih.numep = maxep;

	ep->ep_dn = 0;
	ep->ep_up = 0;
	ep->cdcdn = 0;
	ep->cdcup = 0;
	ep->ready = 0;

	maxep++;

	return ep;
}

int newep2channelup(ep_init_header *eih_in){
struct channel *ch = mychs[0];
struct endpoint *ep = 0;
struct channel *wch=0;
struct {
	ep_data_header edh;
	int	numep;
} epforup;
int ret;

	ep = create_ep(eih_in->addr);

	if (!ep){
		printf("MFI %s error: New endpoint wasn't created\n", appname);
		return -1;
	}

	ch->events = IN_CLOSE;

	printf("\n");

	memcpy((void*) ep->eih.appname, (void*) eih_in->appname, APP_NAME_LEN);
	ep->ep_up = eih_in->numep;

//	printf("MFI %s: Endpoint for asdu id = %d was created\n", appname, ep->eih.addr);

	// Channel connect to endpoint
	wch = findch_by_name(ep->eih.appname);
	if (!wch){
		// Channel not found, start new channel
		wch = newchannelup(pathapp, appname, ep->eih.appname);
		if (wch){
			// Add new channel to up
//			printf("MFI %s: infile %s add to watch %d\n", appname, wch->f_namein, wch->watch);
			wch->watch = inotify_add_watch(d_inoty, wch->f_namein, wch->events);
			ep->cdcup = wch;
//			printf("MFI %s: Channel for %s was created\n", appname, ep->eih.isstr[0]);
			wch->descout = open(wch->f_nameout, O_RDWR);
			//	- two fifos opens! bingo!
		}else return -1;
	}else{
		// Channel found, he exists already
		ep->cdcup = wch;
		epforup.edh.adr = ep->eih.addr;
		epforup.edh.numep = ep->ep_up;
		epforup.edh.len = 4;
		epforup.edh.sys_msg = EP_MSG_NEWEP;
		epforup.numep = ep->my_ep;
		ret=write(wch->descout, &(epforup), sizeof(epforup));
	}

	return 0;
}

// ======================= End Init functions ================================== //



// =================== Default Inotify Callbacks ========================== //
// answer on in_open for init channel
//		- open init channel
int init_open(struct channel *ch){
//	printf("\n-----\nMFI %s: INIT OPEN %s\nActions:\n", appname, ch->appname);

	if (!ch->descin){
		// Open channel
		ch->descin = open(ch->f_namein, O_RDWR);
		if (ch->descin == -1) ch->descin = 0;
		if (ch->descin){
			ch->events &= ~IN_OPEN;
			ch->events |= IN_CLOSE;
//			printf("MFI %s: system has open init file %s, desc = %d\n", appname, ch->f_namein, ch->descin);
		}
	}else{
		printf("MFI %s error: Init channel opens already\n", appname);
	}
	return 0;



//	printf("\nMFI %s: END INIT OPEN %s\n-----\n", appname, ch->appname);

	return 0;
}

// ответная реакция текущего модуля на in_open канала на чтение	(sys_open)
// answer on in_open for working channel:
//      - open channel for reading
//      - open channel for writing
// ответная реакция удаленного модуля на вызов in_open канала на чтение (sys_open)
// other application calls sys_open:
//      - open channel for reading
//      - open channel for writing passed
int sys_open(struct channel *ch){
int ret, i;
struct endpoint *ep = 0; // This endpoint is first for opening up channel in concrete situation
struct {
	ep_data_header edh;
	int	numep;
} epforup;

//	printf("\n-----\nMFI %s: SYSTEM OPEN %s\nActions:\n", appname, ch->appname);

	// Открываем канал на чтение
	if (!ch->descin){
		ch->descin = open(ch->f_namein, O_RDWR | O_NONBLOCK);
		if (ch->descin == -1) ch->descin = 0;
		if (ch->descin){
//			printf("MFI %s: system has open working in file %s, desc = 0x%X\n", appname, ch->f_namein, ch->descin);

			if (ch->descout){
//				// This endpoint is last for opening up channel in concrete situation =>
//				// => Find ep by pointer to up channel
				for(i = (maxep-1); i > 0; i--){
					ep = myeps[i];
					if (ep->cdcup == ch) break;
				}
				if (i == maxep){
					printf("MFI %s error: Endpoint for up channel to %s not found\n", appname, ch->appname);
					return 0;
				}
				epforup.edh.adr = ep->eih.addr;
				epforup.edh.numep = ep->ep_up;
				epforup.edh.len = 4;
				epforup.edh.sys_msg = EP_MSG_NEWEP;
				epforup.numep = ep->my_ep;
				ret=write(ch->descout, &(epforup), sizeof(epforup));

			}
		}
	}

	if (!ch->descout){
		ch->descout = open(ch->f_nameout, O_RDWR);
		if (ch->descout == -1) ch->descout = 0;
		if (ch->descout){
//			printf("MFI %s: system has open working out file %s, desc = 0x%X\n", appname, ch->f_nameout, ch->descout);
		}
	}


//	printf("\nMFI %s: END SYSTEM OPEN %s\n-----\n", appname, ch->appname);
	return 0;
}

// answer on close init channel
int init_close(struct channel *ch){
int ret;
struct endpoint *ep = myeps[maxep-1]; // Last working endpoint
ep_data_header edh;
struct ep_init_header *eih=0;
//	printf("\n-----\nMFI %s: INIT CLOSE\nActions:\n", appname);

	if (ch->descin){
		if (close(ch->descin)){
//			printf("MFI %s: Init file don't close\n", appname);
			return 0;
		}
//		printf("MFI %s: system has closed init file %s\n", appname, ch->f_namein);
		ch->descin = 0;
		ch->events |= IN_OPEN | IN_MODIFY;
		ch->descin = 0;

		ep->ready = 3;

		// Send info about endpoint to up
		edh.adr = ep->eih.addr;
		edh.numep = ep->ep_up;
		edh.len = 0;
		edh.sys_msg = EP_MSG_EPRDY;
		ret=write(ep->cdcup->descout, &edh, sizeof(ep_data_header));

		// Send info about eih to parent thread for continue
		eih = &(ep->eih);
		ret = write(hpp[1], (char*) &eih,  sizeof(int));

	}

//	printf("\nMFI %s: END INIT CLOSE\n-----\n", appname);

	return 0;
}

// answer on close working channel
int sys_close(struct channel *ch){

//	printf("\n-----\nMFI %s: SYSTEM CLOSE %s\nActions:\n", appname, ch->appname);

	if (ch->descin){
		close(ch->descin);
		ch->descin = 0;
//		printf("MFI %s: system has closed working infile %s\n", appname, ch->f_namein);
	}
	if (ch->descout){
		close(ch->descout);
		ch->descout = 0;
//		printf("MFI %s: system has closed working outfile %s\n", appname, ch->f_nameout);
	}
	return 0;

}

// Answer on call in_read of init channel
// Register new endpoint
// Register new channel for mychs[]->name IF NOT
//			- receive endpoint: 5 strings and struct config_device (pathapp, appname, cd->name, cd->protoname, cd->phyname, config_device)
//			- test open channel to mychs[]->name IF NOT:
//								- connect to channel
//								- add fifo to inotify for read
// 								- open fifo for writing
//					 			- two fifos opens! bingo!
//			- endpoint registers
//			- send endpoint to application
int init_read(struct channel *ch){
ep_init_header eih_in;
int len = 0, rdlen;

//	printf("\n-----\nMFI %s: INIT READ %s\nActions:\n", appname, ch->appname);

	// 0 - init already

	if (!ch->descin){
		printf("MFI %s error: init channel closed\n", appname);
		return 0;		// Init file dont open
	}

	rdlen = readchannel(ch);

	if (rdlen == -1){
		ch->events = 0;
		printf("MFI %s error: readchannel init error:%d - %s\n", appname, errno, strerror(errno));
		return 0;
	}


	if (rdlen == -2){
		printf("MFI %s error: ring buffer overflow:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}

	if (rdlen >= sizeof(ep_init_header)){
		// get ep_init_header
		len = getdatafromring(ch, (char*)&eih_in, sizeof(ep_init_header));

		if (len == sizeof(ep_init_header)){
			if (newep2channelup(&eih_in)) return -1;
		}  // len == sizeof(ep_init_header)
	}	// rdlen >= sizeof(ep_init_header)

//	printf("\nMFI %s: END INIT READ\n-----\n", appname);

	return 0;
}

int sys_read(struct channel *ch){
int rdlen, len, ret;
struct endpoint *ep = 0;
struct ep_data_header edh;
struct ep_init_header *eih=0;

//	printf("\n-----\nMFI %s: SYSTEM READ %s\nActions:\n", appname, ch->appname);

	rdlen = readchannel(ch);

	if (rdlen == -1){
		printf("MFI %s error: readchannel system error:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}

	if (rdlen == -2){
		printf("MFI %s error: ring buffer overflow:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}

	len = getdatafromring(ch, (char*) &edh, sizeof(ep_data_header));

	if (len == sizeof(ep_data_header)){

//		printf("MFI %s: ep_data_header_recv:\n- adr=%d\n- numep=%d\n- sysmsg=%d\n- len=%d\n", appname, edh.adr, edh.numep, edh.sys_msg, edh.len);

		if (edh.numep >= maxep){
			printf("MFI %s error: Endpoint %d of %d for receiving data very big\n", appname, edh.numep, maxep);
			return -1;
		}

		// Init ep by recv index
		ep = myeps[edh.numep];
		if (!ep){
			printf("MFI %s error: Endpoint %d for receiving data not found\n", appname, edh.numep);
			return 0;
		}

		if (edh.sys_msg == EP_MSG_NEWEP){
			// Get ep->ep_dn - downlink endpoint's number
			len = getdatafromring(ch, (char*)&ep->ep_dn, sizeof(int));
			// Channel ready to send data
			if (mychs[0]->descout){
	 			ret = close(mychs[0]->descout);
	 			if (ret == -1){
					printf("MFI %s error: init file don't closed: %d - %s\n", appname, errno, strerror(errno));
	 			}
			}else{
				printf("MFI %s error: Handler for init file damaged = 0x%X\n", appname, mychs[0]->descout);
			}
		}

		if (edh.sys_msg == EP_MSG_EPRDY){
 			eih = &(ep->eih);
 			ret = write(hpp[1], (char*) &eih,  sizeof(int));
		}

	}

	if (rdlen){
//		printf("MFI %s: system has read data with rdlen = %d\n", appname, rdlen);
		actchannel = ch;
		if (cb_rcvdata) cb_rcvdata(rdlen);
		actchannel = 0;
	}

	getframefalse(ch);

//	printf("\nMFI %s: END SYSTEM READ %s\n-----\n", appname, ch->appname);

	return 0;
}

// ===================== END Default Inotify Callbacks ======================== //
// ZZzzz

// Thread function for waiting inotify events
int inotify_thr(void *arg){
int i, mask, ret, note;
ssize_t rdlen=0;
struct channel *ch = mychs[0];
struct inotify_event einoty;
static int evcnt=0;
struct timeval tv;
fd_set readset, excpset;

//	printf("MFI %X: start inotify thread OK\n", appname);

	if (!d_inoty){
		printf("MFI %s error: inotify not init\n", appname);
		return -1;
	}
	mychs[0]->watch = inotify_add_watch(d_inoty, mychs[0]->f_namein, mychs[0]->events);

    do{
		// Waiting for inotify events
		FD_ZERO(&readset);
		FD_ZERO(&excpset);
		FD_SET(d_inoty, &readset);
		FD_SET(d_inoty, &excpset);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(d_inoty + 1, &readset, NULL, &excpset, &tv);

		if (FD_ISSET(d_inoty, &excpset)){

			for (i = 0; i < maxch; i++){
				ch = mychs[i];
				if (einoty.wd == ch->watch)	break;
			}
			if (i < maxch) printf("MFI %s: Inotify exception for channel %s\n", appname, ch->appname);
			else  printf("MFI %s: Inotify exception for non-exist channel 0x%X\n", appname, (int) ch);
			exit(0);
		}

		if (FD_ISSET(d_inoty, &readset)){

			rdlen = read(d_inoty, (char*) &einoty, sizeof(struct inotify_event));

			note = 0;
			if (rdlen){
				if (einoty.mask){
					evcnt++;
	//				printf("MFI %s: detect file event: 0x%X in watch %d num %d\n", appname, einoty[note].mask, einoty.wd, evcnt);
					// Find channel by watch
					for (i = 0; i < maxch; i++){
						ch = mychs[i];
						if (einoty.wd == ch->watch){
	//						printf("MFI %s: found channel %d of %d\n", appname, i, maxch);
							break;
						}
					}

					if (i < maxch){
						// Set some events in one
						mask = einoty.mask & ch->events;
	//					printf("MFI event %s: mask: 0x%X[%d]\n", appname, mask, i);

						// Calling callback functions
	//					printf("MFI %s: callback event 0x%X; with event mask 0x%X in watch %d\n", appname, einoty.mask, mask, ch->watch);
						if (mask & IN_OPEN)	ch->in_open(ch);
						if (mask & IN_MODIFY) ch->in_read(ch);
						if (mask & IN_CLOSE) ch->in_close(ch);
					}
				}

				rdlen -= sizeof(struct inotify_event);
				note--;
//				printf("%s: rdlen: %d\n", appname, rdlen);
			}
		}
	}while(inotifystop);
	printf("MFI %s: inotify thread exit!\n", appname);

	return 0;
}


// ================= Signal Handlers  ========================================== //
void sighandler_sigchld(int arg){
	printf("MFI %s: sigchld\n", appname);
	mf_exit();
}

void sighandler_sigquit(int arg){
	printf("MFI %s: own quit\n", appname);
	mf_exit();
	exit(0);
}

// ================= External API ============================================== //
// Create init-channel and run inotify thread for reading files
int mf_init(char *pathinit, char *a_name, void *func_rcvdata){
int ret;
struct channel *initch = 0;
void *stack = NULL;

	signal(SIGQUIT, SIG_IGN);
	signal(SIGPWR, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	appname = malloc(strlen(a_name) + 1);
	strcpy(appname, a_name);
	pathapp = malloc(strlen(pathinit) + 1);
	strcpy(pathapp, pathinit);
	cb_rcvdata = func_rcvdata;

	d_inoty = inotify_init();
	initch = initchannel(pathinit, a_name);
	if (!initch) return -1;
	if (pipe(hpp)) return -1;

	inotifystop = 1;

	stack = (void*) malloc(INOTIFYTHR_STACKSIZE);
	ret =  clone(inotify_thr, stack + INOTIFYTHR_STACKSIZE - sizeof(int), CLONE_VM | CLONE_FS | CLONE_FILES, NULL);

//	signal(SIGQUIT, sighandler_sigquit);
//	signal(SIGCHLD, sighandler_sigchld);

	return ret;
}

void mf_exit(){
	inotifystop = 0;
}

// Form new endpoint
// Return 0: OK
// Return -1: Error. Endpoint not created or channel not created
int mf_newendpoint (u32 addr, char* chld_name, char *pathinit, u32 ep_num){
int ret, wrlen;
//int dninit;
ep_init_header eih_out;
struct channel *ch=0;
struct endpoint *ep=0;
char fname[160];
char *par[2];

	par[0] = chld_name;
	par[1] = NULL;

	ret = testrunningapp(chld_name);

	if(ret == -1) return -1;

	if (!ret){
		// lowlevel application not running
		// running it
		ret = fork();
		if (ret < 0){
			// fork failed
			printf("MFI %s: fork failed: %d - %s\n", appname, errno, strerror(errno));
			exit(0);
		}
		if (!ret){
			// child process
			execv(chld_name, par);
			printf("MFI %s: inotify_init failed: %d - %s\n", appname, errno, strerror(errno));
			exit(0);
		}
		usleep(100000);
	}else{
		printf("MFI %s: LOW LEVEL APPLICATION RUNNING ALREADY\n", appname);
	}

	// Find real endpoint number
	if (ep_num){
		// Forward existing endpoint
		ep = find_ep_by_addr(addr);
		if (!ep){
			printf("MFI %s error: Endpoint with address = %d not found\n", appname, addr);
			return -1;		// if ep_num don't have, ep = 0
		}
		if (ep->cdcdn){
			printf("MFI %s error: Endpoint with address = %d has down-channel already\n", appname, addr);
			return -1;	// if down channel created already
		}
		ep->ready = 0;
	}else{
		// Create new endpoint
		ep = create_ep(addr);
		if (!ep) return -1;		// endpoint don't create
//		printf("%s: Create endpoint begin\n", appname);
	}

	// Find channel for this low level application
	ch = findch_by_name(chld_name);
	if (!ch){
		// Create new channel
		printf("MFI %s: Create channel to %s\n", appname, chld_name);
		ch = newchanneldn(pathinit, chld_name);
		if (!ch) return -1;
//		printf("MFI %s: Channel for %s was created\n", appname, ch->appname);
	}else{
		// Start connect new endpoint with exist channel
		printf("MFI %s: Found channel for %s\n", appname, chld_name);
	}
	ep->cdcdn = ch;

	mychs[0]->descout = 0;

//	printf("MFI %s: Created struct endpoint for asdu id = %d\n", appname, ep->eih.addr);
	// Open init channel for having endpoint
	strcpy(fname, pathinit);
	strcat(fname,"/");
	strcat(fname, chld_name);
	strcat(fname, sufinit);
	mychs[0]->descout = open(fname, O_RDWR);
	if (!mychs[0]->descout){
		printf("MFI %s: Failed to open init file\n", chld_name);
		return -1;
	}

	// prepare ep_init_heaer to transfer to child application
	memcpy((void*) eih_out.appname, (void*) appname, APP_NAME_LEN);
	eih_out.addr = ep->eih.addr;
	eih_out.numep = ep->eih.numep;

	// Write config to init channel
	wrlen = write(mychs[0]->descout, (char*) &(eih_out), sizeof(ep_init_header));

//	printf("\nMFI %s: WAITING THIS ENDPOINT in high level:\n- number = %d\n- up endpoint = %d\n- down endpoint = %d\n", appname, ep->my_ep, ep->ep_up, ep->ep_dn);
//	printf("- up channel desc = 0x%X\n- down channel desc = 0x%X\n\n", (int) ep->cdcup, (int) ep->cdcdn);

	while (mf_waitevent(fname, 160, 5000) != 1);

	mychs[0]->descout = 0;
	ep->ready = 3;

//	printf("MFI %s: connect of endpoint %d for asdu = %d to channel 0x%X (descs = 0x%X/0x%X) completed\n", appname, ep->my_ep, ep->eih.addr, (int) ep->cdcdn, ep->cdcdn->descin, ep->cdcdn->descout);

	return 0;
}


int mf_toendpoint(char *buf, int len, int addr, int direct){
int i, wrlen = 0;
struct channel *ch = 0;
struct endpoint *ep = 0;

	// Find endpoint by addr
	for (i = 1; i < maxep; i++){
		if (myeps[i]->eih.addr == addr){
			ep = myeps[i];
			if (direct == DIRDN){
				ch = ep->cdcdn;
				((struct ep_data_header *)buf)->numep = ep->ep_dn;
			}
			if (direct == DIRUP){
				ch = ep->cdcup;
				((struct ep_data_header *)buf)->numep = ep->ep_up;
			}
			break;
		}
	}

	if (!ep) return -1;
	if (!ch) return -1;

	// Write data to channel
	if (ch->descout) wrlen = write(ch->descout, buf, len);
	else{
		printf("MFI %s error: Outfile descriptor = 0\n", appname);
	}
	if (wrlen == -1) {
		printf("MFI %s error: write error:%d - %s\n",appname, errno, strerror(errno));
		return -1;
	}
	return wrlen;
}


int mf_readbuffer(char *buf, int len, int *addr, int *direct){
int i, ret;
struct endpoint *ep = 0;
struct ep_data_header *edh = 0;
	if (!actchannel) return -1;
	ret = getframefromring(actchannel, buf, len);
	edh = (struct ep_data_header*) buf;
	// Find endpoint
//	printf("MFI %s: readbuffer len=%d\n", appname, len);
	for(i = 1; i < maxep; i++){
		ep = myeps[i];
		if (edh->adr == ep->eih.addr){
			if (ep->cdcdn == actchannel) {*addr = ep->eih.addr; *direct = DIRDN; break;}
			if (ep->cdcup == actchannel) {*addr = ep->eih.addr; *direct = DIRUP; break;}
		}
	}
	return (i==maxep ? 0 : ret);
}

int mf_waitevent(char *buf, int len, int ms_delay){
fd_set rddesc;
int ret;
struct timeval tm;

	FD_ZERO(&rddesc);

	FD_SET(hpp[0], &rddesc);

    if (!ms_delay) ret = select(hpp[1] + 1, &rddesc, NULL, NULL, NULL);
    else{
    	tm.tv_sec = ms_delay / 1000;
    	tm.tv_usec = (ms_delay % 1000) * 1000;
    	ret = select(hpp[1] + 1, &rddesc, NULL, NULL, &tm);
    }

    if (FD_ISSET(hpp[0], &rddesc)){
    	if (ret > 0){
    		ret = read(hpp[0], buf, len);
//    		printf("MFI %s: read error:%d - %s\n",appname, errno, strerror(errno));
    		return 1;
    	}
    }
    return (ms_delay ? 2 : 3);
}
// ================= End External API ============================================== //
