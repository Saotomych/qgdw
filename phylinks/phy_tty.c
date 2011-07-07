/*
 * phy_tty.c
 *
 *  Created on: 05.07.2011
 *      Author: Alex AVAlon
 *
 *      Linked data from physical device (tty) throught parser to main application
 *
 */

#include "../common/common.h"
#include "../common/multififo.h"
#include "local-phyints.h"

// TODO  tty common defines
typedef struct ttydev{
	char devname[15];
	int desc;
	int speed;
	u08 bits;
	u08 stop;
	u08 parity;
	u08 rts;
} TTYDEV;

TTYDEV tdev[8];
int maxtdev;

struct phy_route *myprs[MAXEP];	// One phy_route for one endpoint
struct phy_route *firstpr;
int maxpr = 0;

void CommRawSetup(int hPort, int Speed, int Bits, int Parity, int ParityOdd, int AddStopBit, int RTS){
    struct termios CommOptions;
    tcgetattr(hPort, &CommOptions);//берем атрибуты в структуру для изменения

    cfsetispeed(&CommOptions, Speed);                       //скорость
    cfsetospeed(&CommOptions, Speed);
    CommOptions.c_cflag |= (CLOCAL | CREAD); /* Разрешение приемника и установка  локального режима - всегда обязательно */
    CommOptions.c_cflag &= ~CSIZE;
    CommOptions.c_cflag |= CS8;             // битность

    // Обработка параметров входных
    if (Parity) CommOptions.c_cflag |= PARENB;      // четность есть
    else CommOptions.c_cflag &= ~PARENB;    // четности нет
    if (ParityOdd) CommOptions.c_cflag |= PARODD;   // нечетность есть
    else CommOptions.c_cflag &= ~PARODD;    // нечетности нет
    if (AddStopBit) CommOptions.c_cflag |= CSTOPB;
    else CommOptions.c_cflag &= ~CSTOPB;
    if (RTS) CommOptions.c_cflag |= CRTSCTS;
    else CommOptions.c_cflag &= ~CRTSCTS;   /*для деактивирования аппаратного управления потоком передаваемых данных*/

    /*
    Выбор неканонического (Raw) ввода
    Неканонический ввод не обрабатывается. Вводимый символ передается без изменений, так как он был принят.
    ECHO    Разрешить эхо вводимых символов
    ECHOE    Символ эхо стирания как BS-SP-BS
    ISIG    Разрешить SIGINTR, SIGSUSP, SIGDSUSP, и SIGQUIT сигналы
    */
    CommOptions.c_lflag &= ~(ICANON | ISIG);
    CommOptions.c_iflag &= ~(IXON | IXOFF | IXANY | CMSPAR);/*отмена программно управляемого управления потоком*/
    CommOptions.c_oflag &= ~OPOST;/*Выбор необработанного (raw) вывода*/
    /*
    Драйвера последовательного интерфейса UNIX предоставляют возможность специфицировать таймауты для символа и пакета.
    Два элемента массива c_cc используемых для указания таймаутов: VMIN и VTIME.
    Таймауты игнорируются в случае режима канонического ввода или когда для файла устройства установлена опция NDELAY при открытии файла open или с использованием fcntl.
    VMIN определяет минимальное число символов для чтения. Если VMIN установлено в 0, то значение VTIME определяет время ожидания для каждого читаемого символа.
    Примечательно, что это не подразумевает, что вызов read для N байтов будет ждать поступление N символов.
    Тайаут произойдет в случае задержки приема любого одиночного символа и вызов read вернет число непосредственно доступных символов (вплоть до числа которое вы запросили).
    Если VMIN не 0, то VTIME определяет время ожидания для чтения первого символа.
    Если первый символ прочитан в течение указанного времени, то любое чтение будет блокироваться (ждать) пока все число символов, указанное в VMIN, не будет прочитано.
    Таким образом, как только первый символ будет прочитан, драйвер последовательного интерфейса будет ожидать приема всего пакета символов
    (всего количества байтов указанного в VMIN).
    Если символ не будет прочитан за указанное время, то вызов read вернет 0.
    Этот метод позволяет вам указать последовательному интерфейсу возвращать 0 или N байтов.
    Однако, таймаут будет приниматься только для первого читаемого символа, таким образом, если драйвер по какой-либо причине 'потеряет' один символ из N-байтного пакета,
    то вызов readможет быть заблокирован ожиданием ввода дополнительного символа навсегда.
    VTIME определяет величину времени ожидания ввода символа в десятых долях секунды.
    Если VTIME установлено в 0 (по умолчанию), то чтение будет заблокировано в ожидании на неопределенное время если для порта не была установлена опция NDELAY вызовом open или fcntl. */

    CommOptions.c_cc[VMIN] = 80;                            // 10 байт - размер хидера
    CommOptions.c_cc[VTIME] = 1;                            // таймаут 0.1 сек

    /* Установка параметров порта
    Константа    Описание
    TCSANOW        Произвести изменения немедленно, не дожидаясь завершения операций пересылки данных
    TCSADRAIN    Подождать пока все данные будут переданы
    TCSAFLUSH    Очистить буфера ввода/вывода и произвести изменения
    */
    tcsetattr(hPort, TCSAFLUSH, &CommOptions);      // установили новые атрибуты порта
}

int cfgparse(char *key, char *buf){
char *par;
int i;

	par = strstr(buf, key);
	if (par) par+=6;
	else return 0;

	if (strstr(key, "name")){
		par = strstr(par, "phy_tty");
		if (!par) return -1;
		par += 8;	// par set to name of physical device

		// Find and open device
		for(i=0; i<maxtdev; i++){
			// Find ttydev by name
			if (strstr(tdev[i].devname, par)) break;
		}
		if (i == maxtdev){
			// tty device not found
			strcpy(tdev[maxtdev].devname, "/dev/");
			for (i = 5; (i < 15) && (*par >= '0'); i++){		// copy tty dev name
				tdev[maxtdev].devname[i] = *par; par++;
			}
			tdev[maxtdev].devname[i] = 0;
			i = maxtdev;
			maxtdev++;
		}
		tdev[i].desc = 0; // device descriptor = 0;
		// return index of tty device struct
		return i;
	}

	if (strstr(key, "addr")){
		// return real address of device
		return atoi(par);
	}

	if (strstr(key, "port")){
		// parse device configuration
		return (int)par;
	}

	return -1;
}

void portparse(char *pars, TTYDEV *td){
int t;
	td->speed = atoi(pars);
	pars+=2;
	t = atoi(pars);
	td->bits = t;
	pars++;
	if (*pars == 'N') td->parity = 0;	// R T L
	if (*pars == 'E') td->parity = 1;	// E A E
	if (*pars == 'O') td->parity = 2;	// D B T
	pars++;
	if (*pars == '1') td->stop = 1;
	if (*pars == '2') td->stop = 2;
	pars+=2;
	td->rts = 0;
	if (*pars == 'Y') td->rts = 1;
}

int createroutetable(void){
FILE *addrcfg;
struct phy_route *pr;
char p, *pport;
char outbuf[256];
int i = 1;

// Init physical routes structures by phys.cfg file
	firstpr = malloc(sizeof(struct phy_route) * MAXEP);
	addrcfg = fopen("/rw/mx00/configs/phys.cfg", "r");
	if (addrcfg){
		// Create phy_route tables
		do{
			p = fgets(outbuf, 250, addrcfg);
			if (p){
				// Parse string
				myprs[maxpr] = (struct phy_route*) (firstpr + sizeof(struct phy_route) * maxpr);
				pr = myprs[maxpr];
				pr->asdu = atoi(outbuf);
				if (pr->asdu){
					pr->devindex = cfgparse("-name", outbuf);
					if (pr->devindex != -1){
						pr->realaddr = cfgparse("-addr", outbuf);
						pport = (char*) cfgparse("-port", outbuf);
						portparse(pport, &tdev[pr->devindex]);  // parse port's parameters,
						// init other variables
						pr->ep_index = maxpr;
						pr->state = 0;
						maxpr++;
					}
				}
			}
			i++;
		}while(p);
	}else return -1;
	return 0;
}

int rcvdata(int len){

	return 0;
}

int rcvinit(ep_init_header *ih){
int i, ret;
struct phy_route *pr;

#ifdef _DEBUG
		printf("Phylink TTY: HAS READ INIT DATA: %s\n", ih->isstr[0]);
		printf("Phylink TTY: HAS READ INIT DATA: %s\n", ih->isstr[1]);
		printf("Phylink TTY: HAS READ INIT DATA: %s\n", ih->isstr[2]);
		printf("Phylink TTY: HAS READ INIT DATA: %s\n", ih->isstr[3]);
		printf("Phylink TTY: HAS READ INIT DATA: %s\n", ih->isstr[4]);
		printf("Phylink TTY: HAS READ CONFIG_DEVICE: %d\n\n", ih->addr);
#endif

		// For route struct connect to socket find equal address ASDU in route struct set
		for (i = 0 ; i < maxpr; i++){
			if (myprs[i]->asdu == ih->addr){ pr = myprs[i]; break;}
		}
		if (i == maxpr) return 0;	// Route not found
		if (myprs[i]->state) return 0;	// Route init already
		printf("Phylink TTY: route found: addr = %d, num = %d\n", ih->addr, i);
		pr = myprs[i];

//		pr-> = // uart's descriptor


	return 0;
}

int main(int argc, char * argv[]){
pid_t chldpid;
fd_set rd_socks;
int maxdesc;

	if (createroutetable() == -1){
		printf("Phylink TTY: config file not found\n");
		return 0;
	}
	printf("Phylink TTY: config table ready, %d records\n", maxpr);

	// Init select sets for sockets
	FD_ZERO(&rd_socks);

	// Init multififo
	chldpid = mf_init("/rw/mx00/phyints","phy_tty", rcvdata, rcvinit);



	mf_exit();

	return 0;
}