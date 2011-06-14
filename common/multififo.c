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

// Control channel
char *sufinit = {"-init"};
char *sufdn = {"-dn"};
char *sufup = {"-up"};

static volatile char inotifywork;

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
};


// ============================ Inotify Callbacks ========================== //
// ответная реакция удаленного модуля на вызов in_open init канала	(init_open)
//		- открытие инит канала на чтение +
int init_open(struct channel *ch){
	printf("system has opened init file\n");
	ch->descin = open(ch->f_namein, O_RDWR | O_NDELAY);
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
	printf("system has opened file\n");
	// Открываем канал на чтение
	ch->descin = open(ch->f_namein, O_RDWR | O_NDELAY);
	if (!ch->descout) ch->descout = open(ch->f_nameout, O_RDWR | O_NDELAY);
	return 0;
}

// ответная реакция на закрытие инит-канала = закрытие инит-канала
int init_close(struct channel *ch){
	printf("system has closed init file\n");
	close(ch->descin);
	ch->descin = 0;
	return 0;
}


int sys_close(struct channel *ch){
	printf("system has closed file\n");
	return 0;
}

// ответная реакция удаленного модуля на вызов in_read init канала	(init_read)
//			- проверка наличия открытого канала к конкретному верхнему уровню, если нет, то:
//								- добавление в inotify канала на чтение
//								- открытие канала на запись
// 			- оба канала открыты, принимаем config_device и 3 стринга
// 			- регистрируем endpoint: struct endpoint
//			- отправляем config_device в буфер приложения
int init_read(struct channel *ch){
	printf("system has read init file\n");

	return 0;
}

int sys_read(struct channel *ch){
	printf("system has read file\n");
	return 0;
}

// ========================== END Inotify Callbacks ========================== //


static int maxch;
static struct channel *mychs[8];

void initchannelstruct(int num){
struct channel *ch = mychs[num];
	ch->descin = 0;
	ch->descout = 0;
	ch->mode = S_IFIFO | S_IRUSR  | S_IWUSR;
	ch->events = IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_MODIFY;
	ch->watch = 0;
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

// ================= External API =========================================

// Создает инит канал и запускает поток контроля за каналами на чтение
int mf_init(char *pathinit, char *appname){
int d_inoty, i, mask, ret;
ssize_t rdlen=0;
struct inotify_event einoty;
struct channel *ch;
char namebuf[256];
static int evcnt=0;

	ret = fork();

	inotifywork = 1;

	if (!ret){
		if (initchannel(pathinit, appname)) return -1;
		d_inoty = inotify_init();
		mychs[maxch-1]->watch = inotify_add_watch(d_inoty, mychs[maxch-1]->f_namein, mychs[maxch-1]->events);

		do{
			/* Ждем наступления события или прерывания по отлавливаемому нами
			   сигналу */
			rdlen = read(d_inoty, (char*) &einoty, sizeof(struct inotify_event));
			if (rdlen){
				if (einoty.len){
					read(d_inoty, namebuf, einoty.len);
					namebuf[einoty.len] = 0;
					printf("Name read: %s\n", namebuf);
				}
				evcnt++;
				printf("detect file event: 0x%X in watch %d num %d\n", einoty.mask, einoty.wd, evcnt);
				// Find channel by watch
				for (i = 0; i < maxch; i++){
					ch = mychs[i];
					if (einoty.wd == ch->watch) break;
				}
				if (i < maxch){
					// Calling callback-functions
					mask = einoty.mask & ch->events;
					if (mask & IN_OPEN){
						// Delete channel from inotify
						inotify_rm_watch(d_inoty, ch->watch);
						ch->in_open(ch);
						// Add channel to inotify
						ch->watch = inotify_add_watch(d_inoty, ch->f_namein, ch->events);
					}
					if (mask & IN_CLOSE) ch->in_close(ch);
					if (mask & IN_MODIFY) ch->in_read(ch);
				}
			}
		}while(inotifywork);
	}

	return 0;

}

void mf_exit(){
	inotifywork = 0;
}

int mf_newendpoint (struct config_device *cd){
char pid[160];
char par[160];
int ret;

	strcpy(pid, "/bin/pidof");
	strcpy(par, "~/PRJS/qgdw/u-tsts/multififo-test-device");
//	sprintf(pid, "/bin/pidof %s", cd->name);

	ret = execve(pid, par, NULL);
	printf("fifo: exec:%d - %s\n",errno, strerror(errno));
	printf("%s ret = %d\n",pid,ret);

	return 0;
}

int mf_toendpoint(struct config_device *cd, char *buf, int len){

	return 0;
}

