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

char *appname, *pathapp;

// Control child
pid_t pidchld;

// Control inotify
int d_inoty;

// List of channels
static int maxch;
static struct channel *mychs[8];

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
	u08 ring[1024];
	u08 *bgnframe;	// начало принятого фрейма
	u08 *bgnring;	// начало несчитанных данных принятого фрейма
	u08 *endring;	// конец фрейма
	int rdlen;		// число прочитанных байт блоков данных с момента открытия канала
	int rdstr;		// число прочитанных строк с момента открытия канала
};

// connect device to channel
struct endpoint{
	struct device_config;
	struct channel;
};

// =================== Private functions =================================== //
// Read fifo to ring buffer
int read2channel(struct channel *ch){
u08 *endring = ch->ring + 1024;
int tail;
int rdlen, len;

	if (ch->bgnframe > ch->endring){
		tail = ch->bgnframe - ch->endring;
	}else{
		tail = endring - ch->endring;
	}
	rdlen = read(ch->descin, ch->endring, tail);
	ch->endring += rdlen;
	if (rdlen == tail){
		len = read(ch->descin, ch->ring, count - tail);
		ch->endring = ch->ring + len;
		rdlen += len;
	}
}

// Move string to user buffer
// Frame don't take from ring buffer
int getstringfromring(struct channel *ch, char *buf){
u08 *p, *pbuf;
u08 *endring = ch->ring + 1024;
int l = 0;
	p = ch->bgnring;
	pbuf = buf;
	while((*p) || (p != ch->endring)){
		*pbuf = *p;
		p++; l++;
		if (p >= endring) p = ch->ring;
	}
	if (*p) return -1;
	ch->bgnring = p;
	return l;
}

int getdatafromring(struct channel *ch, char *buf, int len){
int len1, len2;
u08 *endring = ch->ring + 1024;
u08 *end = ch->bgnring + len;
	if (len > 1024) return -1;
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
}

int getframefromring(struct channel *ch, char *buf, int len){
int len1, len2;
u08 *endring = ch->ring + 1024;
u08 *end;
	if (len > 1024) return -1;
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
	// Create FIFO

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
// Подключает его к уже готовым фифо буферам от верхнего уровня
int connect2channel(char *apppath, char *appname){
int len;
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
//			- проверка наличия открытого канала к конкретному верхнему уровню, если нет, то:
//								- добавление в inotify канала на чтение
//								- открытие канала на запись
// 			- оба канала открыты, принимаем 5 стрингов:	pathapp, appname, cd->name, cd->protoname, cd->phyname
//			- и config_device
// 			- регистрируем endpoint: struct endpoint
//			- отправляем config_device в буфер приложения
int init_read(struct channel *ch){
static char *strs[5];
char *nbuf;
int i;
int len, rdlen;
	// 0 - init already
	printf("%s: system has read init file\n", appname);
	rdlen = readtochannel(ch->descin, ch->bgnring, 1024);

	if (rdlen){
		nbuf = malloc(rdlen);
		while(rdlen > 0){
			if (ch->rdstr < 5){
				// get strings
				len = getstringfromring(ch, nbuf, rdlen);
				strs[ch->rdstr] = malloc(len);
				ch->rdstr++;
			}else{
				// get config_device
				len = getdatafromring(ch, nbuf, sizeof(struct device_config));
				if (len == sizeof(struct device_config)) ch->rdlen += rdlen;
				else ch->rdlen = 0;
			}
			rdlen -= len;
			if (len == -1) break;
		}

		free(nbuf);
	}










	if (ch->rdlen > applen){
		nbuf = malloc(applen);
		// find channel in list
		for (i=1; i<8; i++) if (strstr(nbuf, mychs[i]->appname)) break;
		if (i == 8){
			// Channel not found, start new channel
			printf("%s: start new channel from %s\n", appname, nbuf);
			if (!connect2channel(nbuf)){
				// Add new channel to up
				mychs[maxch-1]->watch = inotify_add_watch(d_inoty, mychs[maxch-1]->f_namein, mychs[maxch-1]->events);
				mychs[maxch-1]->descout = open(mychs[maxch-1]->f_nameout, O_RDWR | O_NDELAY);
			}
		}
	}
	printf("%s: common read len = %d\n", appname, ch->rdlen);
	return 0;
}

int sys_read(struct channel *ch){
	printf("%s: system has read file\n", appname);
	return 0;
}

// ===================== END Default Inotify Callbacks ======================== //


// ================= External API ============================================== //

// Создает инит канал и запускает поток контроля за каналами на чтение
int mf_init(char *pathinit, char *a_name){
int i, mask, ret;
ssize_t rdlen=0;
struct inotify_event einoty;
struct channel *ch;
char namebuf[256];
static int evcnt=0;
struct timeval tv;
fd_set readset;


	appname = malloc(strlen(a_name));
	strcpy(appname, a_name);
	pathapp = malloc(strlen(pathinit));
	strcpy(pathapp, pathinit);

	ret = fork();
	if (!ret){
		if (initchannel(pathinit, a_name)) return -1;
		d_inoty = inotify_init();
		mychs[0]->watch = inotify_add_watch(d_inoty, mychs[0]->f_namein, mychs[0]->events);

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
						if (einoty.wd == ch->watch) break;
					}
					if (i < maxch){
						// Calling callback-functions
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
		if (newchannel(pathinit, cd->name)) return -1;
		printf("%s: run app...OK\n", appname);
		sleep(1);
	}else 		printf("low-level application running\n");

	// Open init channel
	strcpy(fname, pathinit);
	strcat(fname,"/");
	strcat(fname,cd->name);
	strcat(fname, sufinit);
	dninit = open(fname, O_RDWR);
	if (!dninit) return -1;

	// Write config to init channel
	wrlen = write(dninit, pathapp, strlen(pathapp));
	wrlen += write(dninit, appname, strlen(appname));
	wrlen += write(dninit, cd->name, strlen(cd->name));
	wrlen += write(dninit, cd->protoname, strlen(cd->protoname));
	wrlen += write(dninit, cd->phyname, strlen(cd->phyname));
	wrlen += write(dninit, &cd, sizeof(struct config_device));

	return 0;
}

int mf_toendpoint(struct config_device *cd, char *buf, int len){

	return 0;
}

// ================= End External API ============================================== //
