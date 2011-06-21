/*
 * multififo-test.c
 *
 *  Created on: 10.06.2011
 *      Author: Alex AVAlon
 *
 *      Каждый из 4ех тестов имитирует поведение центральной программы, девлинка, парсера и физлинка
 *      при взаимодействии с multififo.c
 *
 *  1 вызов - инициализация инит-канала (mf_init)
 *  итог: приложением создан, но никем не открыт инит канал
 *  2 вызов добавляет в приложение нижнего уровня устройство и устанавливает с ним связь (mf_newendpoint (struct config_device *cd))
 *  									- ищет ID приложения нижнего уровня, если не запущен, запускает и
 *																		 создает 2 канала и добавляет в inotify канал на чтение
 *  									- открывает init канал нижнего приложения
 *  									... ожидание открытия канала с той стороны
 * ответная реакция удаленного модуля на вызов in_open init канала	(init_open)
 * 										- открытие инит канала на чтение
 * продолжение выполнения mf_createendpoint
 * 										... открылся
 *  									- запись конфигурации устройства в инит-канал
 * конец выполнения mf_createendpoint
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

#include "../devlinks/devlink.h"
#include "../common/multififo.h"

int rcvdata(char *buf, int len){

	return 0;
}

int rcvinit(char *buf, int len){

	return 0;
}

int main(int argc, char * argv[]){
static const sigset_t sigmask;
pid_t chldpid;
static int wait_st;
int wait_opt = 0;

	chldpid = mf_init("/rw/mx00/devlinks","devlinktest", rcvdata, rcvinit);

	// wait for child process exit
//	sleep(1);
//	printf("mf_devlinktest: chldpid %d\n", chldpid);

//	wait(&wait_st);
//	waitpid(chldpid, &wait_st, wait_opt);
//	if ((int) waitpid(chldpid, &wait_st, wait_opt) == -1){
//		printf("func: waitpid:%d - %s\n",errno, strerror(errno));
//		exit(1);
//	}

	do{
		sigsuspend(&sigmask);
		printf("mf_maintest: detect stop child process with status 0x%X\n", *sigmask.__val);
	}while(*sigmask.__val != SIGQUIT);

//	printf("devlinktest: detect stop child process with status %d\n", wait_st);

//	mf_exit();

	return 0;
}
