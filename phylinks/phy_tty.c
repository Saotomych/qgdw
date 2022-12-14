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
#include "../common/paths.h"
#include "../common/ts_print.h"
#include "local-phyints.h"

static volatile int appexit = 0;	// EP_MSG_QUIT: appexit = 1 => quit application with quit multififo

// tty common defines
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

int Hex2ASCII(char *hexbuf, char *cbuf, int len){
char TmpString[4];
int i;
	cbuf[0]=0;
	for (i=0; i<len; i++){
		snprintf(TmpString, 4, "%02X ", (u08) hexbuf[i]);
		strcat(cbuf, TmpString);
	}
	return len;
}


void CommRawSetup(int hPort, int Speed, int Bits, int Parity, int ParityOdd, int AddStopBit, int RTS){
    struct termios CommOptions;
    tcgetattr(hPort, &CommOptions);//берем атрибуты в структуру для изменения

    cfsetispeed(&CommOptions, Speed);                       //скорость
    cfsetospeed(&CommOptions, Speed);
    CommOptions.c_cflag |= (CLOCAL | CREAD); /* Разрешение приемника и установка  локального режима - всегда обязательно */
    CommOptions.c_cflag &= ~CSIZE;
    CommOptions.c_cflag |= Bits;             // битность

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
    CommOptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* выбор необработанного (raw) ввода*/
    CommOptions.c_iflag &= ~ICRNL; /* отмена конвертации 0x0D в 0x0A (new line to carriage return) */
    CommOptions.c_oflag &= ~ONLCR; /* отмена конвертации 0x0A в 0x0D (carriage return to new line) */
    CommOptions.c_iflag &= ~(IXON | IXOFF | IXANY); /* отмена программно управляемого управления потоком*/
    CommOptions.c_oflag &= ~OPOST; /* выбор необработанного (raw) вывода*/
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

    CommOptions.c_cc[VMIN] = 0;                            // 10 байт - размер хидера
    CommOptions.c_cc[VTIME] = 0;                            // таймаут 0.1 сек

    /* Установка параметров порта
    Константа    Описание
    TCSANOW        Произвести изменения немедленно, не дожидаясь завершения операций пересылки данных
    TCSADRAIN    Подождать пока все данные будут переданы
    TCSAFLUSH    Очистить буфера ввода/вывода и произвести изменения
    */
    tcsetattr(hPort, TCSAFLUSH, &CommOptions);      // установили новые атрибуты порта
}

void send_sys_msg(struct phy_route *pr, int msg){
ep_data_header edh;
	edh.adr = pr->asdu;
	edh.sys_msg = msg;
	edh.len = 0;
	mf_toendpoint((char*) &edh, sizeof(ep_data_header), edh.adr, DIRUP);
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
			// Find ttydev by name in parameters string
			if (strstr(par, tdev[i].devname+5)) break;
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
	t = atoi(pars);
	if (t == 1200) td->speed = B1200;
	if (t == 2400) td->speed = B2400;
	if (t == 9600) td->speed = B9600;
	if (t == 19200) td->speed = B19200;
	if (t == 57600) td->speed = B57600;
	if (t == 115200) td->speed = B115200;
	while(*pars != ',') pars++;
	pars++;
	if (*pars == '5') td->bits = CS5;
	if (*pars == '6') td->bits = CS6;
	if (*pars == '7') td->bits = CS7;
	if (*pars == '8') td->bits = CS8;
	pars++;
	if (*pars == 'N') td->parity = 0;	// R T L
	if (*pars == 'E') td->parity = 1;	// E A E
	if (*pars == 'O') td->parity = 2;	// D B T
	pars++;
	if (*pars == '1') td->stop = 0;
	if (*pars == '2') td->stop = 1;
	pars+=2;
	td->rts = 0;
	if (*pars == 'Y') td->rts = 1;

#ifdef _DEBUG
	ts_printf(STDOUT_FILENO, "Phylink TTY: parse port %s, %d, %d%d%d, rts:%d\n", td->devname, td->speed, td->bits, td->parity, td->stop, td->rts);
#endif
}

int createroutetable(void){
FILE *addrcfg;
char *fname;
struct phy_route *pr;
char *p, *pport;
char outbuf[256];
int i = 1;

// Init physical routes structures by phys.cfg file
	firstpr = malloc(sizeof(struct phy_route) * MAXEP);

	fname = malloc(strlen(getpath2configs()) + strlen("lowlevel.cfg") + 1);
	strcpy(fname, getpath2configs());
	strcat(fname, "lowlevel.cfg");
	addrcfg = fopen(fname, "r");
	free(fname);

	if (addrcfg){
		// Create phy_route tables
		do{
			p = fgets(outbuf, 250, addrcfg);
			if (p){
				// Parse string
				myprs[maxpr] = (struct phy_route*) (firstpr + maxpr);
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
						ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: added device connect to %s, asdu = %d, realadr = %d, %d%d%d - %d\n",
								tdev[pr->devindex].devname, pr->asdu, pr->realaddr, tdev[pr->devindex].bits, tdev[pr->devindex].parity,
								tdev[pr->devindex].stop, tdev[pr->devindex].rts);
					}
				}
			}
			i++;
		}while(p);
	}else return -1;
	return 0;
}

int writeall(int s, char *buf, int len){
int total = 0;
int n;

	while(total < len){
        n = write(s, buf+total, len-total);
        if (n == -1) break;
        total += n;
    }

    return (n==-1 ? -1 : total);
}

int start_ttydevice(TTYDEV *td){
	// open port and init by tdev pars
    td->desc = open(td->devname, O_RDWR | O_NOCTTY | O_NDELAY);

    if (td->desc == -1) {
    	ts_printf(STDOUT_FILENO, TS_INFO, "Phylink TTY:  Can't open tty device %s\n", td->devname);
    	return -1;
    }
    CommRawSetup(td->desc, td->speed, td->bits, td->parity == 1, td->parity == 2, td->stop > 0, td->rts > 0);
    // Read setting with non-blocking
    //	return to blocking ops: fcntl(fd, F_SETFL, 0);
//    fcntl(td->desc, F_SETFL, FNDELAY);

    return td->desc;
}

int rcvdata(int len){
TRANSACTINFO tai;
char *inoti_buf;
struct phy_route *pr;
ep_data_header *edh;
int rdlen, i;
int offset;

char ascibuf[300];

		tai.len = len;
		tai.addr = 0;
		inoti_buf = malloc(len);

		rdlen = mf_readbuffer(inoti_buf, len, &tai.addr, &tai.direct);

		// set offset to zero before loop
		offset = 0;

		while(offset < rdlen)
		{
			if(rdlen - offset < sizeof(ep_data_header))
			{
				free(inoti_buf);
				return 0;
			}

			edh = (struct ep_data_header *) (inoti_buf + offset);				// set start structure
			offset += sizeof(ep_data_header);

			pr = NULL;

			// Get phy_route by addr
			for(i=0; i < maxpr; i++){
				if (myprs[i]->asdu == edh->adr){ pr = myprs[i]; break;}
			}

			// check if phy_route was found
			if (!pr){
				ts_printf(STDOUT_FILENO, TS_INFO, "Phylink TTY: This connection not found\n");
				offset += edh->len;
				continue;
			}

			tai.buf = inoti_buf + offset;	// set pointer to begin data

			ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: Data received. Address = %d, Length = %d, Direction = %s.\n", pr->asdu, len, tai.direct == DIRDN? "DIRUP" : "DIRDN");
			ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: adr=%d sysmsg=%d len=%d\n", edh->adr, edh->sys_msg, edh->len);

			switch(edh->sys_msg){
			case EP_USER_DATA:	// Write data to socket
					if(rdlen-offset >= edh->len && pr->state == 1) {
						Hex2ASCII(tai.buf, ascibuf, edh->len);
						ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: Buffer: %s\n", ascibuf);

//						ty = &tdev[pr->devindex];
//						ts_printf(STDOUT_FILENO, TS_DEBUG, "Device: %s %d%d%d - %d\n", ty->devname, ty->bits, ty->parity, ty->stop, ty->rts);

						writeall(tdev[pr->devindex].desc, tai.buf, edh->len);
					}
					break;

			case EP_MSG_RECONNECT:	// Disconnect and connect according to connect rules for this endpoint
					close(tdev[pr->devindex].desc);
					pr->state = 0;

			case EP_MSG_CONNECT:
					if (start_ttydevice(&tdev[pr->devindex]) == -1) send_sys_msg(pr, EP_MSG_CONNECT_NACK);
					else{
						send_sys_msg(pr, EP_MSG_CONNECT_ACK);
						pr->state = 1;
					}
					break;

			case EP_MSG_CONNECT_CLOSE: // Disconnect endpoint
					break;

			case EP_MSG_QUIT:
					appexit = 1;
					break;
			}

			// move over the data
			offset += edh->len;
		}

		free(inoti_buf);
		return 0;
}


int main(int argc, char * argv[]){
pid_t chldpid;
fd_set rd_desc, ex_desc;
int i, j, maxdesc, ret, rdlen;

TTYDEV *td;
struct phy_route *pr;
ep_data_header *edh;

struct timeval tv;	// for sockets select
char outbuf[300] = {0xFE, 0xFE, 0x68, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x68, 0x13, 0x00, 0xDF, 0x16};

	init_allpaths();
	mf_semadelete(getpath2fifomain(), "phy_tty");

	if (createroutetable() == -1){
		ts_printf(STDOUT_FILENO, TS_INFO, "Phylink TTY: config file not found\n");
		return 0;
	}
	ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: config table ready, %d records\n", maxpr);

// Init multififo
	chldpid = mf_init(getpath2fifophy(),"phy_tty", rcvdata);

	do{
	    FD_ZERO(&rd_desc);
	    FD_ZERO(&ex_desc);
		maxdesc = 0;
		for (i=0; i<maxtdev; i++){
			td = &tdev[i];
			if (td->desc > 0){
				if (td->desc > maxdesc)	maxdesc = td->desc;
				FD_SET(td->desc, &rd_desc);
				FD_SET(td->desc, &ex_desc);
			}
		}

		tv.tv_sec = 1;
	    tv.tv_usec = 0;
	    ret = select(maxdesc + 1, &rd_desc, NULL, &ex_desc, &tv);
	    if (ret == -1) ts_printf(STDOUT_FILENO, TS_INFO, "Phylink TTY: select error:%d - %s\n",errno, strerror(errno));
	    else
	    if (ret){
	    	for (i=0; i<maxtdev; i++){
	    		td = &tdev[i];

				if (FD_ISSET(td->desc, &rd_desc)){
		    		// Read device
		    		rdlen = read(td->desc, outbuf + sizeof(ep_data_header), 300 - sizeof(ep_data_header));

		    		ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: Recv data from tty: ");
		    		for(j=0; j<rdlen; j++){
		    			ts_printf(STDOUT_FILENO, TS_DEBUG, "0x%X ", outbuf[sizeof(ep_data_header) + j]);
		    		}
		    		ts_printf(STDOUT_FILENO, TS_DEBUG, "\n");

    				if (rdlen){
    					// Send to all endpoints connected to this device
    					for(j=0; j<maxpr; j++){
    						pr = myprs[j];
    						if ((pr->state) && (pr->devindex == i)){ 	// connected and live
    							// send buffer to endpoint
    							ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: Reading desc = 0x%X, num = %d, ret = %d, rdlen = %d\n", td->desc, i, ret, rdlen);
    							// Send data frame to endpoint
    							edh = (ep_data_header*) outbuf;
    							edh->adr = pr->asdu;
    							edh->sys_msg = EP_USER_DATA;
    							edh->len = rdlen;
    							ts_printf(STDOUT_FILENO, TS_DEBUG, "Phylink TTY: Send to ASDU num = %d\n", pr->asdu);
    							mf_toendpoint(outbuf, rdlen + sizeof(ep_data_header), pr->asdu, DIRUP);
    						}
		    			}
		    		}
		    	}

		    	if (FD_ISSET(td->desc, &ex_desc)){
					send_sys_msg(pr, EP_MSG_CONNECT_LOST);
					close(td->desc);
					td->desc = 0;
					pr->state = 0;
		    	}
	    	} // for i
	    } // ret
	}while(!appexit);


	mf_exit();

	return 0;
}
