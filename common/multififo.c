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
#include "multififo.h"

#define LENRINGBUF	1024
#define INOTIFYTHR_STACKSIZE	32768

char *appname, *pathapp;

// Control child
volatile static int inotifystop = 0;
volatile int hpp[2];

// Control inotify
static int d_inoty = 0;

// Control channel
char *sufinit = {"-init"};
char *sufdn = {"-dn"};
char *sufup = {"-up"};

int (*cb_rcvdata)(int len);
int (*cb_rcvinit)(ep_init_header *ih);
int (*cb_rcvclose)(char* fn);

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
	int rdlen;		// bytes read since channel opens
	int rdstr;		// strings reads since channel opens
	volatile int ready;		// channel ready to work (sync variable)
};

// connect device to channel
struct endpoint{
	ep_init_header			eih;
	struct channel			*cdcup;
	struct channel			*cdcdn;
	int my_ep;			// unique endpoint's identify application inside
	int ep_up;			// uplevel endpoint's number
	int ep_dn;			// downlevel endpoint's number
};

// List of channels
static int maxch = 0;
static struct channel *mychs[MAXCH];
static struct channel *actchannel;	// Actual channel for data reading

// List of endpoints
static int maxep = 1;
static struct endpoint *myeps[MAXEP];
//static struct endpoint *actep;		// Actual endpoint for work in all functions

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
int ret, wait_st;
char pidof[] = {"/bin/pidof"};
char *par[] = {NULL, name};
char *env[] = {NULL};
int opipe[2];
struct timeval tv;
fd_set readset;
char buf[16];

		if (pipe(opipe) == -1)
		{
			printf("Error: creating pipe");
			return -1;
		}
		par[0] = pidof;
		ret = fork();
		if (!ret){
	        close(1);
	        dup2(opipe[1], 1);
	        close(opipe[0]);
	        close(opipe[1]);
			execve(pidof, par, env);
			printf("MFI %s: fork error:%d - %s\n",appname, errno, strerror(errno));
		}
		waitpid(ret, &wait_st, 0);
		FD_ZERO(&readset);
	    FD_SET(opipe[0], &readset);
	    close(opipe[1]);
	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(opipe[0] + 1, &readset, 0, 0, &tv);
	    if (ret > 0){
	    	return read(opipe[0], buf, 16);
	    }
		waitpid(ret, &wait_st, 0);
	    return -1;
}

struct endpoint *find_ep(u32 num){
int i;
	for(i = 0; i < maxep; i++){
		if (myeps[i]->my_ep == num) return myeps[i];
	}
	return NULL;
}

struct channel *findch_by_name(char *nm){
int i;
	for(i=0; i < maxch; i++){
		if (!strcmp(nm, mychs[i]->appname)) break;
	}
	return (i == maxch ? 0 : mychs[i]);
}

// =================== End Private functions ================================= //

// ======================= Init functions ==================================== //

void initchannelstruct(int num){
struct channel *ch = mychs[num];
	ch->descin = 0;
	ch->descout = 0;
	ch->mode = S_IFIFO | S_IRUSR  | S_IWUSR;
	ch->events = IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_MODIFY;
	ch->watch = 0;
	ch->bgnring = ch->ring;
	ch->bgnframe = ch->ring;
	ch->endring = ch->ring;
	ch->ready = 0; // Not ready
}

// Create init channel, it have index = 0 always
// High level application opens init channel for sending struct config_device => register new endpoint
int initchannel(char *a_path, char *a_name){
int len;

	if (maxch) return -1;
	mychs[0] = malloc(sizeof(struct channel));
	if (!mychs[maxch]) return -1;
	initchannelstruct(0);

	mychs[maxch]->appname = malloc(strlen(a_name));
	strcpy(mychs[maxch]->appname, a_name);

	len = strlen(a_path) + strlen(a_name) + strlen(sufinit) + 8;
	mychs[maxch]->f_namein = malloc(len);
	sprintf(mychs[0]->f_namein, "%s/%s%s", a_path,a_name,sufinit);

	mychs[0]->f_nameout = 0;

	unlink(mychs[0]->f_namein);
	if (mknod(mychs[0]->f_namein, mychs[0]->mode, 0)) return -1;

	mychs[maxch]->in_open = init_open;
	mychs[maxch]->in_close = init_close;
	mychs[maxch]->in_read = init_read;

	maxch++;

	printf("MFI %s: INIT CHANNEL %d READY, MAXCH = %d\n", appname, maxch-1, maxch);
//	printf("MFI %s: init channel file ready: %s\n", appname, mychs[maxch-1]->f_namein);

	return 0;
}

// Create new two-direction channel to downlink
// Channel work for data exchange with lower application
int newchannel(char *a_path, char *a_name){
int len;
	// Create downdirection FIFO

	mychs[maxch] = malloc(sizeof(struct channel));
	if (!mychs[maxch]) return -1;
	initchannelstruct(maxch);

	mychs[maxch]->appname = malloc(strlen(a_name));
	strcpy(mychs[maxch]->appname, a_name);

	len = strlen(a_path) + strlen(a_name) + strlen(sufup) + 8;
	mychs[maxch]->f_namein = malloc(len);
	sprintf(mychs[maxch]->f_namein, "%s/%s%s", a_path, a_name, sufup);

	len = strlen(a_path) + strlen(a_name) + strlen(sufdn) + 8;
	mychs[maxch]->f_nameout = malloc(len);
	sprintf(mychs[maxch]->f_nameout, "%s/%s%s", a_path, a_name, sufdn);

	unlink(mychs[maxch]->f_namein);
	if (mknod(mychs[maxch]->f_namein, mychs[maxch]->mode, 0)) return -1;
	unlink(mychs[maxch]->f_nameout);
	if (mknod(mychs[maxch]->f_nameout, mychs[maxch]->mode, 0)) return -1;

	mychs[maxch]->in_open = sys_open;
	mychs[maxch]->in_close = sys_close;
	mychs[maxch]->in_read = sys_read;

	mychs[maxch]->watch = inotify_add_watch(d_inoty, mychs[maxch]->f_namein, mychs[maxch]->events);

	maxch++;

//	printf("MFI %s: initialize down channel %d ready, MAXCH = %d\n", appname, maxch-1, maxch);
	printf("MFI %s: CHANNEL FILES READY: in - %s & out - %s\n", appname, mychs[maxch-1]->f_namein, mychs[maxch-1]->f_nameout);

	return 0;
}

// Create new two-direction channel to uplink
// Connect to fifos from high level application
// Channel work for data exchange with higher application
int connect2channel(char *a_path, char *a_name){
int len;
	// Create updirection FIFO

	mychs[maxch] = malloc(sizeof(struct channel));

	if (!mychs[maxch]) return -1;
	initchannelstruct(maxch);

	mychs[maxch]->appname = malloc(strlen(a_name));
	strcpy(mychs[maxch]->appname, a_name);

	len = strlen(a_path) + strlen(a_name) + strlen(sufdn) + 8;
	mychs[maxch]->f_namein = malloc(len);
	sprintf(mychs[maxch]->f_namein, "%s/%s%s", a_path, a_name, sufdn);

	len = strlen(a_path) + strlen(a_name) + strlen(sufup) + 8;
	mychs[maxch]->f_nameout = malloc(len);
	sprintf(mychs[maxch]->f_nameout, "%s/%s%s", a_path, a_name, sufup);

	mychs[maxch]->in_open = sys_open;
	mychs[maxch]->in_close = sys_close;
	mychs[maxch]->in_read = sys_read;

	maxch++;

//	printf("MFI %s: connect to up channel %d ready, MAXCH = %d\n", appname, maxch-1, maxch);
	printf("MFI %s: CHANNEL FILES READY: in - %s & out - %s\n", appname, mychs[maxch-1]->f_namein, mychs[maxch-1]->f_nameout);

	return 0;
}

struct endpoint *create_ep(void){
struct endpoint *ep;
	if (maxep > 63) return NULL;
	ep = malloc(sizeof(struct endpoint));
	if (ep > 0){
		myeps[maxep] = ep;
		ep->my_ep = maxep;
		maxep++;
	}else return NULL;

	ep->ep_dn = 0;
	ep->ep_up = 0;
	ep->cdcdn = 0;
	ep->cdcup = 0;
	ep->eih.numep = 0;
	ep->eih.addr = 0;

	return ep;
}

// ======================= End Init functions ================================== //
//
// =================== Default Inotify Callbacks ========================== //
// answer on in_open for init channel
//		- open init channel
int init_open(struct channel *ch){
struct endpoint *ep = 0;
	if (ch->ready) return 0;
	if (!ch->descin){
		// Init open for new endpoint => make new endpoint
		ep = create_ep();
		if (!ep) return -1;
		// Open channel
		ch->descin = open(ch->f_namein, O_RDWR  | O_NDELAY);
		if (ch->descin == -1) ch->descin = 0;
		if (ch->descin){
			ch->events &= ~IN_OPEN;
			ch->events |= IN_CLOSE;
			printf("MFI %s: system has open init file %s\n", appname, ch->f_namein);
			ch->rdlen = 0;
			ch->rdstr = 0;
			ch->ready = 1;
		}
	}

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
int ret;
struct endpoint *ep = myeps[maxep - 1];

	// Открываем канал на чтение
	if (!ch->descin){
		ch->descin = open(ch->f_namein, O_RDWR | O_NDELAY);
		if (ch->descin == -1) ch->descin = 0;
		if (ch->descin){
			printf("MFI %s: system has open working infile %s, desc = 0x%X\n", appname, ch->f_namein, ch->descin);
			ch->rdlen = 0;
			ch->rdstr = 0;
			ch->events &= ~IN_OPEN;
			ch->events |= IN_CLOSE;
			ch->ready += 1;
			if (ch->descout){
				ret=write(ch->descout, &(ep->my_ep), sizeof(int));
				ch->ready = 3;

				printf("MFI %s: READY ENDPOINT in low level:\n- number = %d\n- up endpoint = %d\n- down endpoint = %d\n", appname, ep->my_ep, ep->ep_up, ep->ep_dn);
				printf("- up channel desc = 0x%X\n- down channel desc = 0x%X\n\n", (int) ep->cdcup, (int) ep->cdcdn);

				ret = write(hpp[1], &(ep->my_ep),  sizeof(int));

			}
		}
	}

	if (!ch->descout){
		ch->descout = open(ch->f_nameout, O_RDWR | O_NDELAY);
		if (ch->descout == -1) ch->descout = 0;
		if (ch->descout){
			printf("MFI %s: system has open working outfile %s, desc = 0x%X\n", appname, ch->f_nameout, ch->descout);
			ch->rdlen = 0;
			ch->rdstr = 0;
			ch->events &= ~IN_OPEN;
			ch->events |= IN_CLOSE;
			ch->ready += 1;
		}
	}
	return 0;
}

// answer on close init channel
int init_close(struct channel *ch){
	printf("MFI %s: system has closed init file %s\n", appname, ch->f_namein);
	if (ch->descin){
		close(ch->descin);
		ch->descin = 0;
		ch->events |= IN_OPEN;
		ch->events &= ~IN_CLOSE;
	}
	ch->ready = 0;
	return 0;
}

// answer on close working channel
int sys_close(struct channel *ch){

//	printf("MFI %s: ENDPOINT before SYS_CLOSE:\n- number = %d\n- up endpoint = %d\n- down endpoint = %d\n", appname, ep->my_ep, ep->ep_up, ep->ep_dn);
//	printf("- up channel desc = 0x%X\n- down channel desc = 0x%X\n\n", (int) ep->cdcup, (int) ep->cdcdn);

	if ((ch->descin) && (ch->ready)){
		close(ch->descin);
		ch->descin = 0;
		ch->events |= IN_OPEN;
		ch->events &= ~IN_CLOSE;
		printf("MFI %s: system has closed working infile %s\n", appname, ch->f_namein);
		ch->ready -= 1;
		if (cb_rcvclose) cb_rcvclose(ch->f_namein);		// callback if infile close
	}
	if ((ch->descout) && (ch->ready)){
		close(ch->descout);
		ch->descout = 0;
		ch->events |= IN_OPEN;
		ch->events &= ~IN_CLOSE;
		printf("MFI %s: system has closed working outfile %s\n", appname, ch->f_nameout);
		ch->ready -= 1;
	}
	if (ch->ready > 0) ch->ready = 0;
	return 0;

//	printf("MFI %s: ENDPOINT after SYS_CLOSE:\n- number = %d\n- up endpoint = %d\n- down endpoint = %d\n", appname, ep->my_ep, ep->ep_up, ep->ep_dn);
//	printf("- up channel desc = 0x%X\n- down channel desc = 0x%X\n\n", (int) ep->cdcup, (int) ep->cdcdn);

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
char nbuf[100];
int i;
int len, rdlen;
struct endpoint *ep = myeps[maxep-1];	// new endpoint created in init_open()
ep_init_header *eih;
	// 0 - init already

	if (!ch->ready) return 0;		// Init file dont open
	if (ch->ready == 2) return 0; 	// ep_init_header received already

	rdlen = readchannel(ch);
	if (rdlen == -1){
		printf("MFI %s: readchannel init error:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}
	if (rdlen == -2){
		printf("MFI %s: ring buffer overflow:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}
	if (rdlen){
		printf("\n%s: RING BUFFER READING WITH LEN = %d!!!\n\n", appname, rdlen);
		while(rdlen > 0){
			// Building init pointpp
			if (ch->rdstr < 5){
//				printf("MFI %s: get string number %d\n", appname, ch->rdstr);
				// get strings
				len = getstringfromring(ch, nbuf);
				if (len > 0){
					ep->eih.isstr[ch->rdstr] = malloc(len + 1);
					strcpy(ep->eih.isstr[ch->rdstr], nbuf);
					ch->rdstr++;
					rdlen -= len;
				}else rdlen = 0;
			}else{
				// get config_device
				len = getdatafromring(ch, nbuf, sizeof(ep_init_header));
				eih = (ep_init_header*) nbuf;
				if (len == sizeof(ep_init_header)){
					ep->eih.addr = eih->addr;
					ep->eih.numep = eih->numep;
					ep->ep_up = eih->numep;
					ch->rdlen += len;
				}
				rdlen = 0;
			}
			if (len == -1) rdlen = 0;
		}
		printf("\n%s: END RING BUFFER READING WITH PARS:\n", appname);
		printf("begin frame = %d, begin ring = %d, end ring = %d\n\n", ch->bgnframe-ch->ring, ch->bgnring-ch->ring, ch->endring-ch->ring);

		if ((ch->rdstr == 5) && (ch->rdlen == sizeof(ep_init_header))){
			ch->ready = 2;
//			// Connect to channel
//			printf("MFI %s: connect to working channel... ",appname);
			// find channel in list
			//			- test open channel to mychs[]->name IF NOT:
			//			- connect to channel
			for (i = 1; i < maxch; i++){
				if (mychs[i]){
					if (strstr(ep->eih.isstr[1], mychs[i]->appname)){
//						printf("MFI %s: found channel %d\n", appname, i);
						break;
					}
				}
			}

			if (i == maxch){
				// Channel not found, start new channel
				sprintf(nbuf,"%s/%s",ep->eih.isstr[0], ep->eih.isstr[2]);
//				printf("MFI %s: not found channel for %s\n", appname, ep->eih.isstr[1]);
				if (!connect2channel(ep->eih.isstr[0], ep->eih.isstr[2])){
					// Add new channel to up
					i = maxch-1;
					//	- add fifo to inotify for read
					mychs[i]->watch = inotify_add_watch(d_inoty, mychs[i]->f_namein, mychs[i]->events);
					printf("MFI %s: infile %s add to watch %d\n", appname, mychs[i]->f_namein, mychs[i]->watch);
					//	- open fifo for writing
					mychs[i]->descout = open(mychs[i]->f_nameout, O_RDWR | O_NDELAY);
					printf("MFI %s: outfile opens %s\n", appname, mychs[i]->f_nameout);
					//	- two fifos opens! bingo!
					ep->cdcup = mychs[i];
				}else return -1;
			}
//			// Call callback function for working config_device
			ep->eih.isstr[2] = ep->eih.isstr[3];
			ep->eih.isstr[3] = ep->eih.isstr[4];
			if (cb_rcvinit) cb_rcvinit(&(ep->eih));
			getframefalse(ch);

		} // rdstr == 5 .....
	}	// rdlen != 0

	return 0;
}

int sys_read(struct channel *ch){
int rdlen, len, numep;
struct endpoint *ep = myeps[maxep-1];

	if (!ch->ready) return 0;
	rdlen = readchannel(ch);
//	printf("MFI %s: system has read data with rdlen = %d\n", appname, rdlen);

	if (rdlen == -1){
		printf("MFI %s: readchannel system error:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}

	if (rdlen == -2){
		printf("MFI %s: ring buffer overflow:%d - %s\n", appname, errno, strerror(errno));
		return -1;
	}

	if (ch->ready < 3){
		// Get ep->ep_dn - downlink endpoint's number
		len = getdatafromring(ch, (char*) &numep, sizeof(int));
		if (len == sizeof(int)) memcpy(&(ep->ep_dn), &numep, sizeof(int));
		printf("MFI %s: READY ENDPOINT in high level:\n- number = %d\n- up endpoint = %d\n- down endpoint = %d\n", appname, ep->my_ep, ep->ep_up, ep->ep_dn);
		printf("- up channel desc = 0x%X\n- down channel desc = 0x%X\n\n", (int) ep->cdcup, (int) ep->cdcdn);
		// Channel ready to send data
		getframefalse(ch);
		ch->ready = 3;
		rdlen -= len;
	}

	if (rdlen){
		actchannel = ch;
		if (cb_rcvdata) cb_rcvdata(rdlen);
		getframefalse(ch);
		actchannel = 0;
	}

	return 0;
}

// ===================== END Default Inotify Callbacks ======================== //
// ZZzzz

// Thread function for waiting inotify events
int inotify_thr(void *arg){
int i, mask, ret;
ssize_t rdlen=0;
struct inotify_event einoty;
struct channel *ch = mychs[0];
char namebuf[256];
static int evcnt=0;
struct timeval tv;
fd_set readset;

//	printf("MFI %s: start inotify thread OK\n", appname);

	if (!d_inoty){
		printf("MFI %s: inotify not init\n", appname);
		return -1;
	}
//	printf("MFI %s: init inotify ready, desc=0x%X\n", appname, d_inoty);
	mychs[0]->watch = inotify_add_watch(d_inoty, mychs[0]->f_namein, mychs[0]->events);
//	printf("MFI %s: init channel %s in watch %d\n", appname, mychs[0]->f_namein, mychs[0]->watch);

	do{
		// Waiting for inotify events
		FD_ZERO(&readset);
		FD_SET(d_inoty, &readset);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = select(d_inoty + 1, &readset, 0, 0, &tv);
		if (ret > 0){
			rdlen = read(d_inoty, (char*) &einoty, sizeof(struct inotify_event));
			if (rdlen){
				if (einoty.len){
					rdlen = read(d_inoty, namebuf, einoty.len);
					if (rdlen > 0){
						namebuf[rdlen] = 0;
//						printf("MFI %s: name read: %s\n", appname, namebuf);
					}
				}
				evcnt++;
//				printf("MFI %s: detect file event: 0x%X in watch %d num %d\n", appname, einoty.mask, einoty.wd, evcnt);
				// Find channel by watch
				for (i = 0; i < maxch; i++){
					ch = mychs[i];
					if (einoty.wd == ch->watch){
//						printf("MFI %s: found channel %d of %d\n", appname, i, maxch);
						break;
					}
				}
				if (i < maxch){
					// Calling callback functions
					mask = einoty.mask & ch->events;
//					printf("MFI %s: callback event 0x%X; with event mask 0x%X in watch %d\n", appname, einoty.mask, mask, ch->watch);
					if (mask & IN_OPEN)	ch->in_open(ch);
					if (mask & IN_CLOSE) ch->in_close(ch);
					if (mask & IN_MODIFY) ch->in_read(ch);
				}
			}
		}
	}while(inotifystop);
//	printf("MFI %s: inotify thread exit!\n", appname);

	return 0;
}


// ================= Signal Handlers  ========================================== //
void sighandler_sigchld(int arg){
	printf("mf_maintest: sigchld\n");
	mf_exit();
}

void sighandler_sigquit(int arg){
	printf("mf_maintest: own quit\n");
	mf_exit();
	exit(0);
}

// ================= External API ============================================== //
// Create init-channel and run inotify thread for reading files
char stack[INOTIFYTHR_STACKSIZE];
int mf_init(char *pathinit, char *a_name, void *func_rcvdata, void *func_rcvinit){
int ret;

	signal(SIGQUIT, SIG_IGN);
	signal(SIGPWR, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	appname = malloc(strlen(a_name));
	strcpy(appname, a_name);
	pathapp = malloc(strlen(pathinit));
	strcpy(pathapp, pathinit);
	cb_rcvdata = func_rcvdata;
	cb_rcvinit = func_rcvinit;

	d_inoty = inotify_init();
	if (initchannel(pathinit, a_name)) return -1;
	if (pipe(hpp)) return -1;

	inotifystop = 1;

	ret =  clone(inotify_thr, (void*)(stack+INOTIFYTHR_STACKSIZE-1), CLONE_VM | CLONE_FS | CLONE_FILES, NULL);

	signal(SIGQUIT, sighandler_sigquit);
//	signal(SIGCHLD, sighandler_sigchld);

	return ret;
}

void mf_exit(){
	inotifystop = 0;
}

// Form new endpoint
// Return 0: OK
// Return -1: Error. Endpoint not created or cnahhel not created
int mf_newendpoint (struct config_device *cd, char *pathinit, u32 ep_num){
int ret, wrlen;
char fname[160];
int dninit;
struct channel *ch;
struct endpoint *ep;

	if (!testrunningapp(cd->name)){
		// lowlevel application not running
		// running it

		if (newchannel(pathinit, cd->name)) return -1;

//		printf("MFI %s: RUN LOW LEVEL APPLICATION. WAITING INITIALIZATION... ", appname);
		ret = fork();
		if (!ret){
			ret = execve(cd->name, NULL, NULL);
//			printf("MFI %s: inotify_init:%d - %s\n", appname, errno, strerror(errno));
			exit(0);
		}

		// TODO sleep exchange to one pipe
		sleep(1);
		printf("OK\n");

	}else 		printf("RUNNING LOW LEVEL APPLICATION\n");

	if (ep_num){
		// Forward existing endpoint
//		ep = find_ep(ep_num);
		ep = myeps[maxep-1];	// Forwarding last endpoint
		if (!ep) return -1;		// if ep_num don't have, ep = 0
		if (ep->cdcdn) return -1;	// if down channel created already

	}else{
		// Create new endpoint
		ep = create_ep();
		if (!ep) return -1;		// endpoint don't create
		// Init new endpoint
		ep->eih.isstr[0] = malloc(strlen(pathinit) + 1);
		strcpy(ep->eih.isstr[0], pathinit);
		ep->eih.isstr[1] = appname;
		ep->eih.isstr[2] = cd->name;
		ep->eih.isstr[3] = cd->protoname;
		ep->eih.isstr[4] = cd->phyname;

//		printf("%s: Create endpoint begin\n", appname);

	}

	// Find channel for this low level application
	ch = findch_by_name(cd->name);
	ep->cdcdn = ch;

	// Open init channel for having endpoint
//	strcpy(fname, ep->eih.isstr[0]);
	strcpy(fname, pathinit);
	strcat(fname,"/");
	strcat(fname, ep->eih.isstr[2]);
	strcat(fname, sufinit);
	dninit = open(fname, O_RDWR);

//	printf("MFI %s: init open %s\n", appname, fname);

	if (!dninit) return -1;

	ep->eih.addr = cd->addr;
	ep->eih.numep = maxep-1;
	// Write config to init channel
	wrlen  = write(dninit, ep->eih.isstr[0], strlen(pathinit)+1);
	wrlen += write(dninit, ep->eih.isstr[1], strlen(appname)+1);
	wrlen += write(dninit, ep->eih.isstr[2], strlen(cd->name)+1);
	wrlen += write(dninit, ep->eih.isstr[3], strlen(cd->protoname)+1);
	wrlen += write(dninit, ep->eih.isstr[4], strlen(cd->phyname)+1);
	wrlen += write(dninit, &(ep->eih), sizeof(ep_init_header));

//	printf("\nMFI %s: WAITING THIS ENDPOINT in high level:\n- number = %d\n- up endpoint = %d\n- down endpoint = %d\n", appname, ep->my_ep, ep->ep_up, ep->ep_dn);
//	printf("- up channel desc = 0x%X\n- down channel desc = 0x%X\n\n", (int) ep->cdcup, (int) ep->cdcdn);

	while(ch->ready < 2);
	close(dninit);

	while(ch->ready < 3);
	printf("MFI %s: new endpoint completed\n", appname);

	return 0;
}

// Send data to Endpoint by addr and to direction (DIRUP || DIRDN - priority)
// Return writing length: OK
// Return -1: Error. Endpoint not created or not exist or Cnahhel not exist
int mftai_toendpoint(TRANSACTINFO *tai){
	if (tai->addr) return mf_toendpoint(tai->buf, tai->len, tai->addr, tai->direct);
	else return mf_toendpoint_by_index(tai->buf, tai->len, tai->ep_index, tai->direct);
}

int mf_toendpoint_by_index(char *buf, int len, int index, int direct){
int wrlen;
struct channel *ch = 0;
struct endpoint *ep = myeps[index];
fd_set rd, ex;

	if (!ep) return -1;

	if (direct == DIRDN) ch = ep->cdcdn;
	if (direct == DIRUP) ch = ep->cdcup;
	if (!ch) return -1;
	if (ch->ready < 3) return -1;

	wrlen = write(ch->descout, buf, len);
	if (wrlen == -1){
		printf("MFI %s: write error:%d - %s\n",appname, errno, strerror(errno));
		if (errno == 11){	// Resource temporarily unavailable
			FD_ZERO(&rd); FD_ZERO(&ex);
			FD_SET(ch->descout, &rd);
			FD_SET(ch->descout, &ex);
			if (select(ch->descout, &rd, NULL, &ex, NULL) > 0)
				if (write(ch->descout, buf, len) == -1){
					printf("MFI %s: write error:%d - %s\n",appname, errno, strerror(errno));
					return -1;
				}
		}
	}
	return wrlen;
}

int mf_toendpoint(char *buf, int len, int addr, int direct){
int i, wrlen;
struct channel *ch = 0;
struct endpoint *ep = 0;
//	printf("MFI %s: mf2endpoint start, maxch = %d, maxep = %d\n", appname, maxch, maxep);

	// Find endpoint by addr
	for (i = 1; i < maxep; i++){
		if (myeps[i]->eih.addr == addr){
			ep = myeps[i];
			if (direct == DIRDN) ch = ep->cdcdn;
			if (direct == DIRUP) ch = ep->cdcup;
			break;
		}
	}

	if (!ep) return -1;
	if (!ch) return -1;
	if (ch->ready < 3) return -1;

//	printf("MFI %s: find channel for writing %d of %d\n", appname, i, maxch-1);
	// Write data to channel
	wrlen = write(ch->descout, buf, len);
	if (wrlen == -1) {
		printf("MFI %s: write error:%d - %s\n",appname, errno, strerror(errno));
		return -1;
	}
	return wrlen;
}

// Read data from actual channel, set functions
// Return:
// int real reading length
// unit unique addr by pointer
// direction of received data by pointer
int mftai_readbuffer(TRANSACTINFO *tai){
	if (tai->addr) return mf_readbuffer(tai->buf, tai->len, &(tai->addr), &(tai->direct));
	else return mf_readbuffer_by_index(tai->buf, tai->len, &(tai->ep_index), &(tai->direct));
}

int mf_readbuffer_by_index(char *buf, int len, int *index, int *direct){
	int i;
	struct endpoint *ep;
		if (!actchannel) return -1;
		// Find endpoint
	//	printf("MFI %s: readbuffer len=%d\n", appname, len);
		for(i = 1; i < maxep; i++){
			ep = myeps[i];
			if (ep->cdcdn == actchannel) {*index = i; *direct = DIRDN; break;}
			if (ep->cdcup == actchannel) {*index = i; *direct = DIRUP; break;}
		}
		return getframefromring(actchannel, buf, len);
}

int mf_readbuffer(char *buf, int len, int *addr, int *direct){
int i;
struct endpoint *ep;
	if (!actchannel) return -1;
	// Find endpoint
//	printf("MFI %s: readbuffer len=%d\n", appname, len);
	for(i = 1; i < maxep; i++){
		ep = myeps[i];
		if (ep->cdcdn == actchannel) {*addr = ep->eih.addr; *direct = DIRDN; break;}
		if (ep->cdcup == actchannel) {*addr = ep->eih.addr; *direct = DIRUP; break;}
	}
	return getframefromring(actchannel, buf, len);
}

void mf_set_cb_rcvclose(void *func_rcvclose){
	cb_rcvclose = func_rcvclose;
}

int mf_waitevent(char *buf, int len, int ms_delay){
fd_set rddesc, exdesc;
int ret;

	FD_ZERO(&rddesc);
    FD_ZERO(&exdesc);
	FD_SET(hpp[0], &rddesc);
	FD_SET(hpp[0], &exdesc);

    ret = select(hpp[1] + 1, &rddesc, NULL, &exdesc, NULL);

    if (FD_ISSET(hpp[0], &exdesc)) return 0;

	if (FD_ISSET(hpp[0], &rddesc)){
		ret = read(hpp[0], buf, len);
		return 1;
	}

	return 2;

}
// ================= End External API ============================================== //
