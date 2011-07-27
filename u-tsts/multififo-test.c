/*
 * multififo-test.c
 *
 *  Created on: 10.06.2011
 *      Author: Alex AVAlon
 *
 *
 *  1 вызов - инициализация инит-канала (mf_init)
 *  итог: приложением создан, но никем не открыт инит канал
 *
 *  2 вызов добавляет в приложение нижнего уровня устройство и устанавливает с ним связь (mf_newendpoint (struct config_device *cd))
 *  									- ищет ID приложения нижнего уровня, если не запущен, запускает и
 *																		 создает 2 канала и добавляет в inotify канал на чтение
 *  									- открывает init канал нижнего приложения
 * приостановка выполнения mf_createendpoint в блокировке функции open
 * ответная реакция удаленного модуля на вызов in_open init канала	(init_open)
 * 										- инит-канал еще не открыт, то открытие инит канала на чтение
 * продолжение выполнения mf_createendpoint
 *  									- инит-канал уже открыт, то запись конфигурации устройства в инит-канал
 * ответная реакция удаленного модуля на вызов in_read init канала	(init_read)
 *  									- проверка наличия открытого канала к конкретному верхнему уровню, если нет, то:
 *  											- добавление в inotify канала на чтение
 *			  									- открытие канала на запись
 *  									ответная реакция текущего модуля на in_open канала на чтение	(sys_open)
 *			  									- ответное открытие канала на чтение
 *  											- здесь же открытие канала на запись, т.к. он еще не открыт
 *	 	 	 	 	 	 	 	 	 	ответная реакция удаленного модуля на вызов in_open канала на чтение (sys_open)
 *			  									- открытие канала на чтение
 *  											- канал на запись уже открыт, пропускаем шаг
 *  									- оба канала открыты, принимаем config_device и 3 стринга
 *									 	- регистрируем endpoint: struct endpoint
 *										- отправляем config_device в буфер приложения
 *
 *  3 вызов - посылает блок данных в канал (mf_toendpoint(struct config_device *cd, char *buf, int len))
 *  ответная реакция удаленного модуля на in_read канала на чтение - помещение блока данных в буфер
 *
 *  реакция на вызов in_read канала на чтение (sys_read)
 *  									- помещение данных в буфер
 *
 *  4 вызов - выход из цикла опроса inotify (mf_exit)
 *
 */

#include "../common/common.h"
#include "../common/multififo.h"

char testdata[] = {"Do you see this string? Data received, all right.\0"};

//char devlink[] = {"multififo-test-device"};
char devlink[] 		= {"phy_tcp"};
char protoname[] 	= {"prototest"};
char physlink[] 	= {"phy_tcp"};

char *appname;

struct config_device cd = {
		devlink,
		protoname,
		physlink,
		10,
};

int rcvdata(int len){
char buf[100];
int addr;
int dir;

	mf_readbuffer(buf, len, &addr, &dir);
	printf("\nMF_TEST HAS READ DATA: %s\n\n", buf);

	return 0;
}

int rcvinit(ep_init_header *ih, int len){

	return 0;
}

static sigset_t sigmask;
int main(int argc, char * argv[]){
pid_t chldpid;
int exit = 0;
int i;

	appname = malloc(strlen(argv[0]));
	strcpy(appname, argv[0]);

	chldpid = mf_init("/rw/mx00/devlinks", appname, rcvdata);


	for(i=0; i<64;i++){

	mf_newendpoint(&cd, "/rw/mx00/phyints", 0);

	mf_toendpoint(testdata, strlen(testdata)+1, cd.addr, DIRDN);

	printf("open %i endpoint\n", i);

	}

	do{
		sigsuspend(&sigmask);
	}while(!exit);

	mf_exit();

	return 0;
}
