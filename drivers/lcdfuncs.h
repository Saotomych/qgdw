// Universal pointers for connecting lcd driver to am160160_drv.c
/*      init
 * 		exit
 *      writecmd
 *      writedat
 *      readinfo
 *      readdata
 */

typedef struct _AMLCDFUNC{
	void (*init)(void);											// Initialize hardware driver
	void (*writecmd)(unsigned char cmd);						// Write command to hardware driver
	void (*writedat)(unsigned char *buf);		// Write data buffer to hardware driver
	unsigned long (*readinfo)(void);								// Return pointer to info buffer
	unsigned char* (*readdata)(unsigned char *addr, unsigned int len);				// Return length & pointer to data buffer
	unsigned int (*exit)(void);									// Return 0 if release resources OK
} AMLCDFUNC, *PAMLCDFUNC;

extern PAMLCDFUNC uc1698_connect(unsigned char *io_c, unsigned char *io_d,  unsigned int len);
extern PAMLCDFUNC st7529_connect(unsigned char *io_c, unsigned char *io_d,  unsigned int len);
