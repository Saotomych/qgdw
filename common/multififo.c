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
pid_t pidchld;

// Control inotify
int d_inoty;

// Control channel
char *sufinit = {"-init"};
char *sufdn = {"-dn"};
char *sufup = {"-up"};


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
	char *bgnframe;	// начало принятого фрейма
	char *bgnring;	// начало несчитанных данных принятого фрейма
	char *endring;	// конец фрейма
	int rdlen;		// число прочитанных байт блоков данных с момента открытия канала
	int rdstr;		// число прочитанных строк с момента открытия канала
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

// Создает инит-канал, он всегда нулевой
// Инит-канал открывается приложением следующего верхнего уровня для подачи системных команд
int initchannel(char *apppath, char *appname){
int len;
	if (maxch) return -1;
	mychs[0] = malloc(sizeof(struct channel));
	if (!mychs[maxch]) return -1;
	initchannelstruct(0);

	mychs[maxch]->appname = malloc(strlen(appname));
	strcpy(mychs[maxch]->appname, appname);

	len = strlen(apppath) + strlen(appname) + strlen(sufinit) + 8;
	mychs[maxch]->f_namein = malloc(len);
	sprintf(mychs[0]->f_namein, "%s/%s%s", apppath,appname,sufinit);

	mychs[0]->f_nameout = 0;

	unlink(mychs[0]->f_namein);
	if (mknod(mychs[0]->f_namein, mychs[0]->mode, 0)) return -1;

	mychs[maxch]->in_open = init_open;
	mychs[maxch]->in_close = init_close;
	mychs[maxch]->in_read = init_read;

	maxch++;

	return 0;
}

// Создает новый двунаправленный именованный канал
// Создаваемый канал предназначен для активации связи с приложением следующего нижнего уровня
// После создания канала по инит-каналу приложения следующего нижнего уровня  ему будет выдана
// конфигурация канала, который нужно захватить для обмена, здесь это будет делать функция connectchannel
int newchannel(char *apppath, char *appname){
int len;
	// Create downdirection FIFO

	mychs[maxch] = malloc(sizeof(struct channel));
	if (!mychs[maxch]) return -1;
	initchannelstruct(maxch);

	mychs[maxch]->appname = malloc(strlen(appname));
	strcpy(mychs[maxch]->appname, appname);

	len = strlen(apppath) + strlen(appname) + strlen(sufup) + 8;
	mychs[maxch]->f_namein = malloc(len);
	sprintf(mychs[maxch]->f_namein, "%s/%s%s", apppath, appname, sufup);

	len = strlen(apppath) + strlen(appname) + strlen(sufdn) + 8;
	mychs[maxch]->f_nameout = malloc(len);
	sprintf(mychs[maxch]->f_nameout, "%s/%s%s", apppath, appname, sufdn);

	unlink(mychs[maxch]->f_namein);
	if (mknod(mychs[maxch]->f_namein, mychs[maxch]->mode, 0)) return -1;
	unlink(mychs[maxch]->f_nameout);
	if (mknod(mychs[maxch]->f_nameout, mychs[maxch]->mode, 0)) return -1;

	mychs[maxch]->in_open = sys_open;
	mychs[maxch]->in_close = sys_close;
	mychs[maxch]->in_read = sys_read;

	maxch++;

	return 0;
}

// Создает новый двунаправленный именованый канал
// Подключает его к уже готовым фифо буферам от верхнего уровня	*pbuf=0;

int connect2channel(char *apppath, char *appname){
int len;
	// Create updirection FIFO

	mychs[maxch] = malloc(sizeof(struct channel));

	if (!mychs[maxch]) return -1;
	initchannelstruct(maxch);

	mychs[maxch]->appname = malloc(strlen(appname));
	strcpy(mychs[maxch]->appname, appname);

	len = strlen(apppath) + strlen(appname) + strlen(sufdn) + 8;
	mychs[maxch]->f_namein = malloc(len);
	sprintf(mychs[maxch]->f_namein, "%s/%s%s", apppath, appname, sufdn);

	len = strlen(apppath) + strlen(appname) + strlen(sufup) + 8;
	mychs[maxch]->f_nameout = malloc(len);
	sprintf(mychs[maxch]->f_nameout, "%s/%s%s", apppath, appname, sufup);

	mychs[maxch]->in_open = sys_open;
	mychs[maxch]->in_close = sys_close;
	mychs[maxch]->in_read = sys_read;

	maxch++;
	return 0;
}

// ======================= End Init functions ================================== //
//
// =================== Default Inotify Callbacks ========================== //
// ответная реакция удаленного модуля на вызов in_open init канала	(init_open)
//		- открытие инит канала на чтение
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
//		- ответное открытие канала на чтение +
//  	- здесь же открытие канала на запись, т.к. он еще не открыт +
// ответная реакция удаленного модуля на вызов in_open канала на чтение (sys_open)
//		- открытие канала на чтение	+
//  	- канал на запись уже открыт, пропускаем шаг +
int sys_open(struct channel *ch){
//int ret;
	printf("%s: system has opened file\n", appname);
	// Открываем канал на чтение
	ch->descin = open(ch->f_namein, O_RDWR | O_NDELAY);
	if (!ch->descout) ch->descout = open(ch->f_nameout, O_RDWR | O_NDELAY);
	return 0;
}

// ответная реакция на закрытие инит-канала = закрытие инит-канала
int init_close(struct channel *ch){
	printf("%s: system has closed init file\n", appname);
	if (ch->descin)	close(ch->descin);
	ch->descin = 0;
	ch->events |= IN_OPEN;
	ch->events &= ~IN_CLOSE;
	return 0;
}


int sys_close(struct channel *ch){
	printf("%s: system has closed file\n", appname);
	return 0;
}

// ответная реакция удаленного модуля на вызов in_read init канала	(init_read)
// Регистрация нового ендпойта всегда, нового канала, если его еще нет.
// 			- прием конфигурации каналов для ендпойнта: 5 стрингов и структуру:	pathapp, appname, cd->name, cd->protoname, cd->phyname, config_device
//			- проверка наличия открытого канала к конкретному верхнему уровню, если нет, то:
//								- регистрация подключения к новому
//								- добавление в inotify канала на чтение
//								- открытие канала на запись
//					 			- оба канала открыты! bingo!
// 			- регистрируем endpoint: struct endpoint, то есть канал и конфигурацию вместе
//			- отправляем config_device в буфер приложения
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
			// Building init point
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
		printf("%s: connect to channel\n",appname);
		// find channel in list
		//			- проверка наличия открытого канала к конкретному верхнему уровню, если нет, то:
        //								- регистрация подключения к новому
        //								- добавление в inotify канала на чтение
        //								- открытие канала на запись
        // 			- оба канала открыты! bingo!
		for (i=1; i<16; i++){
			if (mychs[i])
				if (strstr(isstr[1], mychs[i]->appname)) break;
		}

		if (i == 16){
			// Channel not found, start new channel
			sprintf(nbuf,"%s/%s",isstr[0], isstr[2]);
			printf("%s: start new channel - %s\n", appname, nbuf);
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

// ================= External API ============================================== //

// Создает инит канал и запускает поток контроля за каналами на чтение
int mf_init(char *pathinit, char *a_name){
int i, mask, ret;
ssize_t rdlen=0;
struct inotify_event einoty;
struct channel *ch = mychs[0];
char namebuf[256];
static int evcnt=0;
struct timeval tv;
fd_set readset;

	appname = malloc(strlen(a_name));
	strcpy(appname, a_name);
	pathapp = malloc(strlen(pathinit));
	strcpy(pathapp, pathinit);

	if (initchannel(pathinit, a_name)) return -1;
	d_inoty = inotify_init();

	ret = fork();
	if (!ret){
		if (!d_inoty){
			printf("%s: inotify not init\n", appname);
			return -1;
		}

		mychs[0]->watch = inotify_add_watch(d_inoty, mychs[0]->f_namein, mychs[0]->events);
		printf("%s: init 0x%X\n", appname, d_inoty);

		do{
			/* Ждем наступления события или прерывания по отлавливаемому нами
			   сигналу */
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
					// Find channel by watch
					for (i = 0; i < maxch; i++){
						ch = mychs[i];
						printf("%s: find channel %d of %d\n", appname, i, maxch);
						if (einoty.wd == ch->watch) break;
					}
					if (i < maxch){
						// Calling callback-functions	printf("%s: newep 0x%X\n", appname, d_inoty);

						mask = einoty.mask & ch->events;
						if (mask & IN_OPEN)	ch->in_open(ch);
						if (mask & IN_CLOSE) ch->in_close(ch);
						if (mask & IN_MODIFY) ch->in_read(ch);
					}
				}
			}
		}while(1);
		printf("Exit!\n");
	}

	pidchld = ret;

	return 0;
}

void mf_exit(){
	kill(pidchld, SIGKILL);
}

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

	printf("%s: newep 0x%X %d\n", appname, d_inoty, maxch);
	if (newchannel(pathinit, cd->name)) return -1;

	// Add watch in(up) channel
	printf("%s: newep 0x%X %d\n", appname, d_inoty, maxch);

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

	return 0;
}

int mf_toendpoint(struct config_device *cd, char *buf, int len){

	return 0;
}

// ================= End External API ============================================== //