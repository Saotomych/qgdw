/*
 * multififo.c
 *
 *  Created on: 09.06.2011
 *      Author: Alex AVAlon
 *
 * Control connections to/from applications by pair fifos
 *
 */

#include "../devlinks/devlink.h"
#include "multififo.h"

#define LENRINGBUF	1024

char *appname, *pathapp;

// Control child
volatile static int inotifystop = 0;

// Control inotify
static int d_inoty = 0;

// Control channel
char *sufinit = {"-init"};
char *sufdn = {"-dn"};
char *sufup = {"-up"};

int (*cb_rcvdata)(char *buf, int len);
int (*cb_rcvinit)(char *buf, int len);

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
};

// connect device to channel
struct endpoint{
	struct config_device *edc;
	struct channel		 *cdc;
};

// Init channel datas
char *isstr[5];
struct config_device *idc;

// List of channels
static int maxch = 0;
static struct channel *mychs[16];

// List of endpoints
static int maxep = 0;
static struct endpoint *myeps[64];

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
int read2channel(struct channel *ch){
char *endring = ch->ring + LENRINGBUF;
int tail;
int rdlen, len;
int notreadlen;

	if (ch->bgnring > ch->endring){
		tail = ch->bgnring - ch->endring;
		notreadlen = ch->endring + LENRINGBUF - ch->bgnring ;
	}else{
		tail = endring - ch->endring;
		notreadlen = ch->endring - ch->bgnring;
	}

	rdlen = read(ch->descin, ch->endring, tail);

	ch->endring += rdlen;
	if (rdlen == tail){
		len = read(ch->descin, ch->ring, ch->bgnframe - ch->ring);
		ch->endring = ch->ring + len;
		rdlen += len;
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
char *end;
	if (len > LENRINGBUF) return -1;
	end = ch->bgnframe + len;
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

	return 0;
}

int testrunningapp(char *name){
int ret;
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
			printf("fifo: exec:%d - %s\n",errno, strerror(errno));
		}

		FD_ZERO(&readset);
	    FD_SET(opipe[0], &readset);
	    close(opipe[1]);
	    tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(opipe[0] + 1, &readset, 0, 0, &tv);
	    if (ret > 0){
	    	return read(opipe[0], buf, 16);
	    }
	    return -1;
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

	printf("%s: init channel %d ready\n", appname, maxch);

	maxch++;

	printf("%s: MAXCH = %d\n", appname, maxch);

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

	printf("%s: initialize down channel %d ready. files: in - %s & out - %s\n", appname, maxch, mychs[maxch]->f_namein, mychs[maxch]->f_nameout);

	maxch++;

	printf("%s: MAXCH = %d\n", appname, maxch);

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

	printf("%s: connect to up channel %d ready. files: in - %s & out - %s\n", appname, maxch, mychs[maxch]->f_namein, mychs[maxch]->f_nameout);

	maxch++;

	printf("%s: MAXCH = %d\n", appname, maxch);

	return 0;
}

// ======================= End Init functions ================================== //
//
// =================== Default Inotify Callbacks ========================== //
// answer on in_open for init channel
//		- open init channel
int init_open(struct channel *ch){
	printf("%s: system has opened init file\n", appname);
	if (!ch->descin){
		ch->descin = open(ch->f_namein, O_RDWR  | O_NDELAY);
		if (ch->descin != -1){
			ch->events &= ~IN_OPEN;
			ch->events |= IN_CLOSE;
		}else ch->descin = 0;
		ch->rdlen = 0;
		ch->rdstr = 0;
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
//int ret;
	printf("%s: system has opened working file\n", appname);
	// Открываем канал на чтение
	if (!ch->descin){
		ch->descin = open(ch->f_namein, O_RDWR | O_NDELAY);
		ch->rdlen = 0;
		ch->rdstr = 0;
	}
	if (!ch->descout){
		ch->descout = open(ch->f_nameout, O_RDWR | O_NDELAY);
	}
	return 0;
}

// answer on close init channel
int init_close(struct channel *ch){
	printf("%s: system has closed init file\n", appname);
	if (ch->descin)	close(ch->descin);
	ch->descin = 0;
	ch->events |= IN_OPEN;
	ch->events &= ~IN_CLOSE;
	return 0;
}

// answer on close working channel
int sys_close(struct channel *ch){
	printf("%s: system has closed working file\n", appname);
	if (ch->descin)	close(ch->descin);
	ch->descin = 0;
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
char nbuf[100];
int i;
int len, rdlen;
	// 0 - init already
	printf("%s: system has read init file\n", appname);
	rdlen = read2channel(ch);
	if (rdlen){

		printf("%s: string in buffer \"%s\" \n", appname, ch->ring);

		while(rdlen > 0){
			// Building init pointpp
			if (ch->rdstr < 5){
				printf("%s: get string number %d\n", appname, ch->rdstr);
				// get strings
				len = getstringfromring(ch, nbuf);
				printf("%s: get string \"%s\"; number %d; len %d;\n", appname, nbuf, ch->rdstr, len);
				if (len > 0){
					isstr[ch->rdstr] = malloc(len);
					strcpy(isstr[ch->rdstr], nbuf);
					printf("%s: %d - %s\n", appname, ch->rdstr, isstr[ch->rdstr]);
					ch->rdstr++;
					rdlen -= len;
				}else rdlen = 0;
			}else{
				// get config_device
				len = getdatafromring(ch, nbuf, sizeof(struct config_device));
				if (len == sizeof(struct config_device)){
					idc = malloc(sizeof(struct config_device));
					memcpy(idc, nbuf, sizeof(struct config_device));
					ch->rdlen += len;
				}
				rdlen = 0;
			}
			printf("%s: rdlen=%d\n",appname,rdlen);
			if (len == -1) rdlen = 0;;
		}
	}

	if ((ch->rdstr == 5) && (ch->rdlen == sizeof(struct config_device))){
		// Connect to channel
		printf("%s: connect to working channel... ",appname);
		// find channel in list
		//			- test open channel to mychs[]->name IF NOT:
		//								- connect to channel
		//								- add fifo to inotify for read
		// 								- open fifo for writing
		//					 			- two fifos opens! bingo!
		for (i = 1; i < maxch; i++){
			if (mychs[i]){
				if (strstr(isstr[1], mychs[i]->appname)){
					printf("found channel %d\n", i);
					break;
				}
			}
		}

		if (i == maxch){
			// Channel not found, start new channel
			sprintf(nbuf,"%s/%s",isstr[0], isstr[2]);
			printf("not found channel for %s\n", isstr[1]);
			if (!connect2channel(isstr[0], isstr[2])){
				// Add new channel to up
				i = maxch-1;
				mychs[i]->watch = inotify_add_watch(d_inoty, mychs[i]->f_namein, mychs[i]->events);
				printf("%s: infile %s add to watch %d\n", appname, mychs[i]->f_namein, mychs[i]->watch);
				mychs[i]->descout = open(mychs[i]->f_nameout, O_RDWR | O_NDELAY);
				printf("%s: outfile opens %s\n", appname, mychs[i]->f_nameout);
			}else return -1;
		}

		// Building endpoint
		myeps[maxep] = malloc(sizeof(struct endpoint));
		myeps[maxep]->cdc = mychs[i];
		myeps[maxep]->edc = idc;

		// Send SIGNAL config_device to application buffer
		// will be there
	}

	return 0;
}

int sys_read(struct channel *ch){
	printf("%s: system has read file\n", appname);
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

	printf("%s: start inotify thread OK\n", appname);

	if (!d_inoty){
		printf("%s: inotify not init\n", appname);
		return -1;
	}
	printf("%s: init inotify ready, desc=0x%X\n", appname, d_inoty);
	mychs[0]->watch = inotify_add_watch(d_inoty, mychs[0]->f_namein, mychs[0]->events);
	printf("%s: init channel in watch %d\n", appname, mychs[0]->watch);

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
						printf("%s: name read: %s\n", appname, namebuf);
					}
				}
				evcnt++;
				printf("%s: detect file event: 0x%X in watch %d num %d\n", appname, einoty.mask, einoty.wd, evcnt);
				printf("%s: MAXCH in waiting = %d\n", appname, maxch);
				// Find channel by watch
				for (i = 0; i < maxch; i++){
					ch = mychs[i];
					if (einoty.wd == ch->watch){
						printf("%s: find channel %d of %d\n", appname, i, maxch);
						break;
					}
				}
				if (i < maxch){
					// Calling callback functions
					mask = einoty.mask & ch->events;
					printf("%s: callback 0x%X\n", appname, mask);
					if (mask & IN_OPEN)	ch->in_open(ch);
					if (mask & IN_CLOSE) ch->in_close(ch);
					if (mask & IN_MODIFY) ch->in_read(ch);
				}
			}
		}
	}while(inotifystop);
	printf("Exit!\n");

	return 0;
}


// ================= External API ============================================== //
// Create init-channel and run inotify thread for reading files
char stack[10000];
int mf_init(char *pathinit, char *a_name, void *func_rcvdata, void *func_rcvinit){
	appname = malloc(strlen(a_name));
	strcpy(appname, a_name);
	pathapp = malloc(strlen(pathinit));
	strcpy(pathapp, pathinit);
	cb_rcvdata = func_rcvdata;
	cb_rcvinit = func_rcvinit;

	d_inoty = inotify_init();
	if (initchannel(pathinit, a_name)) return -1;

	printf("%s: start thread\n", appname);

	inotifystop = 1;

	return clone(inotify_thr, (void*)(stack+10000-1), CLONE_VM /*| CLONE_FS | CLONE_FILES*/, NULL);

//	if (pidchld == -1){
//		printf("%s: clone:%d - %s\n", appname, errno, strerror(errno));
//        exit(1);
//	}
//	return pidchld;
}

void mf_exit(){
	inotifystop = 0;
}

// Form new endpoint
int mf_newendpoint (struct config_device *cd, char *pathinit){
int ret, wrlen;
char fname[160];
int dninit;

	if (!testrunningapp(cd->name)){
		// lowlevel application not running
		// running it
		ret = fork();
		if (!ret){
			ret = execve(cd->name, NULL, NULL);
			printf("%s: inotify_init:%d - %s\n", appname, errno, strerror(errno));
		}
		printf("%s: run app...OK\n", appname);
		sleep(1);
	}else 		printf("low-level application running\n");

	if (newchannel(pathinit, cd->name)) return -1;

	// Add watch in(up) channel
	mychs[maxch-1]->watch = inotify_add_watch(d_inoty, mychs[maxch-1]->f_namein, mychs[maxch-1]->events);
	printf("%s: infile add to watch %d, %s\n", appname, mychs[maxch-1]->watch, mychs[maxch-1]->f_namein);

	// Open init channel
	strcpy(fname, pathinit);
	strcat(fname,"/");
	strcat(fname,cd->name);
	strcat(fname, sufinit);
	dninit = open(fname, O_RDWR);
	if (!dninit) return -1;

	// Write config to init channel
	wrlen  = write(dninit, pathinit, strlen(pathinit)+1);
	wrlen += write(dninit, appname, strlen(appname)+1);
	wrlen += write(dninit, cd->name, strlen(cd->name)+1);
	wrlen += write(dninit, cd->protoname, strlen(cd->protoname)+1);
	wrlen += write(dninit, cd->phyname, strlen(cd->phyname)+1);
	wrlen += write(dninit, &cd, sizeof(struct config_device));
	printf("%s: %d bytes writes to %s\n", appname, wrlen, fname);

	return 0;
}

int mf_toendpoint(struct config_device *cd, char *buf, int len){

	return 0;
}

// ================= End External API ============================================== //
