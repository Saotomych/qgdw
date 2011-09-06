/*
 * autoconfig.c
 *
 *  Created on: 01.09.2011
 *      Author: Alex AVAlon
 */

#include "../common/common.h"
#include "../common/multififo.h"

typedef struct lowrecord{
	char 		cfg[128];
	uint16_t	asdu;
	u08			connect;
	u08 		copied;
	uint32_t	ldinst;
	uint32_t	addr;
	uint32_t	addrdlt;
	uint16_t	port;
	uint16_t	scen;		// Case for type lowlevel string
} LOWREC;

LOWREC lrs[MAXEP];

uint32_t maxrec = 0, actrec = 0;

uint16_t	lastasdu;
uint16_t	lastldinst;

char iec104={"unitlink-iec104"};
char iec101={"unitlink-iec101"};
char intern={"unitlink-m700"};
char tcp={"phy_tcp"};
char tty={"phy_tty"};

// E-Meters
char *kipp={"kipp"};
char *m100={"m100"};
char *m300={"m300"};
char *m500={"m500"};
char *m700={"m700"};

inline char *finddig(char *p){
	while ((*p) && (*p <= '9') && (*p >= '0')) p++;
	return p;
}

int findasdu(uint16_t){

	return 0;
}

void setstring104(char *paddr, LOWREC *lr){

}

void setstringdlt(char *paddr, LOWREC *lr){

}

void setstring101(char *paddr, LOWREC *lr){

}

void setstringintern(char *paddr, LOWREC *lr){

}

char *makellstring(char *paddr, LOWREC *lr){
long testval;
u08 i;
char *p;
	// Detect scenario
	if (strncasecmp(kipp, paddr, 4)){
		lr->addr = 0;
		lr->port = 0;

		// Detect chain (unitlink - phylink)
		p = paddr;
		// Skip 2 "
		while(*p != '"') p++;
		while(*p != '"') p++;
		p = finddig(p);

		i=0;
		do{
			testval = atol(p);
			p = finddig(p);
			if (*p){
				if ((*(p-1) == '.') && (i < 3)) lr->addr |= (testval & 0xFF) << i;
				if ((*(p-1) == ':') && (i == 3)) lr->addr |= (testval & 0xFF) << 3;
			}else lr->asdu = testval;
			i++;
		}while (testval);
		if (i == 4) lr->port = testval;

		// Set asdu

		// Set ld.inst

		return lr->cfg;
	}

	if (strncasecmp(m100, paddr, 4)){

		return lr->cfg;
	}

	if (strncasecmp(m300, paddr, 4)){

		return lr->cfg;
	}

	if (strncasecmp(m500, paddr, 4)){

		return lr->cfg;
	}

	if (strncasecmp(m700, paddr, 4)){

		return lr->cfg;
	}
}


int createroutetable(void){
FILE *addrcfg;
struct phy_route *pr;
char *p, *pport;
char outbuf[256];
int i = 1;

	lastasdu = 1;
	lastldinst = 0;

// Init physical routes structures by phys.cfg file
	firstpr = malloc(sizeof(struct phy_route) * MAXEP);
	addrcfg = fopen("/rw/mx00/configs/lowlevel.cfg", "r");
	if (addrcfg){
		// Create table lowlevel.cfg
		// 1 - all for iec-101 & iec-104
		// 2 - m700 & m500
		// 3 - other
		do{
			p = fgets(outbuf, 250, addrcfg);
			actrec = 0;
			if (p){
				// Для каждой строчки addr.cfg составляем строчку lowlevel.cfg и делаем так MAXEP строк
				// Parse string addr.cfg

				// Для каждого tty проверяем 3 раза на скоростях 9600, 2400, 1200
				// Create strings for lowlevel.cfg from 1 string of addr.cfg

				// Прокидываем enpoints

				// Wait asdu answer

			}
			i++;
		}while(p);
	}else return -1;
	return 0;
}

int main(int argc, char * argv[]){

	// Backup previous lowlevel.cfg
	rename("/rw/mx00/configs/lowlevel.cfg", "/rw/mx00/configs/lowlevel.bak");
	// Create full table for all addr.cfg records
	createroutetable();

	// Start endpoints

	// Control of answers and forming new lowlevel.cfg

	// Exit by time

	// write new lowlevel.cfg

	// Quit all lowlevel applications


}
