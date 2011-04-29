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
	void (*writedat)(unsigned char *buf, unsigned int len);		// Write data buffer to hardware driver
	unsigned int (*readinfo)(void);								// Return pointer to info buffer
	unsigned int (*readdata)(unsigned char *addr);				// Return length & pointer to data buffer
	unsigned int (*exit)(void);									// Return 0 if release resources OK
	unsigned int resetpin;
	unsigned int lightpin;
} AMLCDFUNC, *PAMLCDFUNC;

extern PAMLCDFUNC uc1698_connect(void);
extern PAMLCDFUNC st7529_connect(void);
