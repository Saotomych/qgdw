/*
 * linux/drivers/video/skeletonfb.c -- Skeleton for a frame buffer device
 *
 *  Modified to new api Jan 2001 by James Simmons (jsimmons@transvirtual.com)
 *
 *  Created 28 Dec 1997 by Geert Uytterhoeven
 *
 *  am160160_drv - driver for indicator am160160 Andorin with support fb
 *  Created by 04.04.2011
 *  Author: alex AAV
 *
 *  Make console to framebuffer
 *  Switch mode console to graphic made by writing 1-bit color bitmap to /dev/fb0
 *	Switch mode graphic to console made by writing any 1-7 bytes to /dev/fb0
 *	In console mode writing to /dev/tty0
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/console.h>
#include <linux/vt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/fb.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <mach/at91sam9_smc.h>

#include "am160160_drv.h"
#include "lcdfuncs.h"

#undef CONFIG_PCI
#undef CONFIG_PM

#define PIXMAP_SIZE	1
#define BUF_LEN		80*160
#define VID_LEN		3200

// Vars for ttyprintk
struct tty_struct *my_tty;

static char defchipname[]={"uc1698"};
static char *chipname = defchipname;
module_param_string(chip, defchipname, 7, 0);

unsigned char *mapvd;					// Income data buffer mapped to user space

static unsigned char video[BUF_LEN];	// Graphics video buffer
static unsigned char tmpvd[BUF_LEN];
static unsigned char convideo[BUF_LEN];	// Console video buffer

#define AMFB_CONSOLE_MODE	1
#define AMFB_GRAPH_MODE		2
static unsigned char	am_fbmode;

static PAMLCDFUNC hard = 0;
unsigned char *io_cmd, *io_data;								// virtual i/o indicator addresses
static struct console *am160160fbcon;

/* Board depend */
static struct resource *am160160_resources[3];
static struct platform_device *am160160_device;
static unsigned int resetpin;
static unsigned int lightpin;

/*
 * Driver data
 */
static struct vm_operations_struct am160160_vm_ops;
static char *mode_option __initdata;
struct am160160_par;

static int device_cnt=0;

static struct fb_fix_screeninfo am160160_fb_fix __devinitdata = {
	.id =		"am160160",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_MONO01,
	.xpanstep =	0,
	.ypanstep =	0,
	.ywrapstep = 0,
	.accel =	FB_ACCEL_NONE,
	.line_length = 20,
};

//static struct fb_monspecs am_monspecs = {
//
//};

static struct fb_videomode def_fb_videomode = {
	.name = "160x160-1@6",
	.refresh = 6,
	.xres = 160,
	.yres = 160,
	.vmode = FB_VMODE_CONUPDATE,
	.flag = FB_IGNOREMON,
};

// Refresh timer, vars.
#define TICKSMAX		7
struct timer_list sync_timer;
static unsigned char counter = TICKSMAX;
// Refresh func
void sync_timer_func(unsigned long data){
unsigned int x, bt;
unsigned char *pvideo = video;
unsigned char mask, i;

	if (am_fbmode == AMFB_GRAPH_MODE){

	    // Decode to indicator format from 2color bmp
	    memset(video, 0, BUF_LEN);
	    for (x=0; x < VID_LEN; x++){
	    	mask = 0x80;
			bt = mapvd[x];
	    	for (i=0; i<8; i++){
	    		if (bt & mask) pvideo[(x<<2)+(i>>1)] |= ((i & 1) ? 0x8 : 0x80);
	    		mask >>= 1;
	    		pvideo += (i&1);
	    	}
	    }

	    hard->writedat(video, BUF_LEN);

	}

	mod_timer(&sync_timer, jiffies + HZ/TICKSMAX);
}

void am160160_vma_open(struct vm_area_struct *vma);
void am160160_vma_close(struct vm_area_struct *vma);
int am160160_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf);

unsigned char constring[1024];
static void myprintk(void){

	if (my_tty != NULL) {
	    ((my_tty->driver)->ops)->write(my_tty, constring, strlen(constring));
	}
}

static int am160160_fb_open(struct fb_info *info, int user){

	if (user > 0) am_fbmode = AMFB_GRAPH_MODE;
	else am_fbmode = AMFB_CONSOLE_MODE;

	sprintf(constring, KERN_INFO "fb_open(%d), fb mode = %d\n\r", user, am_fbmode);
	myprintk();

	return 0;
}

// (struct fb_info *info, char __user *buf,  size_t count, loff_t *ppos);
static ssize_t am160160_fb_read(struct fb_info *info, char __user *buffer, size_t length, loff_t *offset){
	unsigned int len=0, i;
	unsigned char *prddata = 0;

	len = (int) hard->readdata(prddata, BUF_LEN);
	if ((prddata) && (len)){
		for(i=0; i<length && i < info->fix.smem_len; i++) put_user(prddata[i], (char __user *) buffer + i);
		sprintf(constring, KERN_INFO "fb_read(%p,%p,%d)\n\r",info,buffer,length);
		myprintk();
	}

	return len;
}

static ssize_t am160160_fb_write(struct fb_info *info, const char __user *buffer, size_t length, loff_t *offset){
    size_t rlen = info->fix.mmio_len;
//    unsigned long pos = (unsigned long) offset;
    char *pvideo = video;
    unsigned int x, i;
    unsigned char mask;

	if (length < 8) am_fbmode = AMFB_CONSOLE_MODE;

	sprintf(constring, KERN_INFO "fb_write(%p,%p,%d,%lX)\n\r",info,buffer,length, (long unsigned int)offset);
	myprintk();

	// Вместо офсета в реале приходит какая-то эпическая хуйня, поэтому пока не юзаем
	// Выяснить где и как оно выставляется и юзается.

	// Проверка на переход смещения за границу буфера и на приход блока с длиной 0
	//    if (pos >= info->fix.smem_len || (!length)) return length;
	if (!length) return 0;

    // Если пришедший блок короче, чем выделенная память, то ставим длину этого блока.
	// Обычно ожидается приход блока равный памяти
    if (length <= info->fix.smem_len) rlen = length;
    // Если текущая длина + смешщение больше, чем выделенная память, то вычитаем смещение
    //    if (rlen + pos > info->fix.smem_len) rlen-=pos;

    // Читаем в буфер блок данных
    if (copy_from_user(tmpvd, (const char __user *) buffer, rlen)) return -EFAULT;

    // Decode to indicator format from 2color bmp
    memset(video, 0, BUF_LEN);
    for (x=0; x<rlen; x++){
    	mask = 0x80;
    	for (i=0; i<8; i++){
    		if (tmpvd[x] & mask){
    			pvideo[(x<<2)+(i>>1)] |= ((i & 1) ? 0x8 : 0x80);
    		}
    		mask >>= 1;
    	}
    }

    hard->writedat(pvideo, rlen << 2);

	return rlen;
}

static int am160160_fb_release(struct fb_info *info, int user){

	sprintf(constring, KERN_INFO "fb_release(%d), fb mode = %d\n\r", user, am_fbmode);
	myprintk();

	return 0;
}

//	according to sys_imageblit(pinfo, image)
//  Console graphics inbound to convideo buffer
static void am160160_fb_imageblit(struct fb_info *pinfo, const struct fb_image *image){

unsigned int fg = image->fg_color, bg = image->bg_color, bt;
unsigned int dx = image->dx, dy = image->dy;
unsigned int w = image->width, h = image->height;
unsigned int ll = pinfo->fix.line_length;

// Start & end addrs of console screen
unsigned int adrstart, adrstop, lenx;

// Pointers to video data in and out
char *pdat = (char *) image->data;
unsigned char *pvideo;

unsigned int x, y, i;
unsigned char mask;

	lenx = w >> 3;	// y нас всегда кратна 8
	if ((fg ^ bg) & fg) fg = 0;
	else fg = 0xFF;

//	sprintf(constring, KERN_INFO "dx:%d, dy:%d, bpp:%d, bg:0x%X, fg:0x%X, w:%d, h:%d\n\r", dx, dy, image->depth, bg, fg, w, h);
//	myprintk();

	// Clean low console string
	memset(&convideo[12160], fg, 640);

    for (y = 0; y < h; y++){
    	adrstart = lenx * y;
    	adrstop = adrstart + lenx;
    	pvideo = convideo + ((dy + y) * (ll<<2)) + (dx >> 1);
    	for (x = adrstart; x < adrstop; x++){
    		mask = 0x80;
			bt = pdat[x] ^ fg;
    		for (i=0; i<8; i++){
    			if (bt & mask)	*pvideo |= ((i & 1) ? 0x8 : 0x80);
    			else     			*pvideo &= ~((i & 1) ? 0x8 : 0x80);
    			mask >>= 1;
    			pvideo += (i&1);
    		}
    	}
    }

	if (am_fbmode == AMFB_GRAPH_MODE) return;

	if (hard) hard->writedat(convideo, BUF_LEN);
}

static void am160160_fb_fillrect(struct fb_info *pinfo, const struct fb_fillrect *rect){
unsigned int dx = rect->dx, dy = rect->dy;
unsigned int w = rect->width, h = rect->height;
unsigned int color = rect->color;
unsigned int ll = pinfo->fix.line_length;

// Start & end addrs of console screen
unsigned int adrstart, adrstop, lenx;

// Pointers to video data in and out
//char *pdat = (char *) image->data;
//char *pvideo = pinfo->screen_base;
unsigned char *pvideo;

unsigned int x, y, i;
unsigned char mask;

	if (am_fbmode == AMFB_GRAPH_MODE) return;
	sprintf(constring, KERN_INFO "fb_fillrect enter\n\r");
	myprintk();
	//lenx = w >> 3;	// y нас всегда кратна 8
	//if ((fg ^ bg) & fg) fg = 0;
	//else fg = 0xFF;

//	sprintf(constring, KERN_INFO "dx:%d, dy:%d, bpp:%d, bg:0x%X, fg:0x%X, w:%d, h:%d\n\r", dx, dy, image->depth, bg, fg, w, h);
//	myprintk();

    for (y = 0; y < h; y++){
    	adrstart = lenx * y;
    	adrstop = adrstart + lenx;
    	pvideo = convideo + ((dy + y) * (ll<<2)) + (dx >> 1);
    	for (x = adrstart; x < adrstop; x++){
    		mask = 0x80;
    		for (i=0; i<8; i++){
    			if (color & mask)	*pvideo |= ((i & 1) ? 0x8 : 0x80);
    			else     			*pvideo &= ~((i & 1) ? 0x8 : 0x80);
    			mask >>= 1;
    			pvideo += (i&1);
    		}
    	}
    }
}

static void am160160_fb_copyarea(struct fb_info *pinfo, const struct fb_copyarea *area){

   unsigned int dx = area->dx, dy = area->dy;
   unsigned int sx = area->sx, sy = area->sy;
   unsigned int height = area->height, width = area->width;

   if (am_fbmode == AMFB_GRAPH_MODE) return;
}

static long am160160_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg){
long ret = 0;
struct fb_fix_screeninfo fix;
struct fb_var_screeninfo var;
struct fb_cmap_user cmap;

	sprintf(constring, KERN_INFO "fb_ioctl enter\n\r");
	myprintk();

	switch(cmd){
	case FBIOGET_FSCREENINFO:

		sprintf(constring, KERN_INFO "fb_ioctl, FBIOGET_FSCREENINFO\n\r");
		myprintk();

		if (!lock_fb_info(info)) return -ENODEV;
		fix = info->fix;
		unlock_fb_info(info);

		ret = copy_to_user((void __user *) arg, &fix, sizeof(fix)) ? -EFAULT : 0;
		break;

	case FBIOGET_VSCREENINFO:

		sprintf(constring ,KERN_INFO "fb_ioctl, FBIOGET_VSCREENINFO\n\r");
		myprintk();

		if (!lock_fb_info(info)) return -ENODEV;
		var = info->var;
		unlock_fb_info(info);

		ret = copy_to_user((void __user *) arg, &var, sizeof(var)) ? -EFAULT : 0;
		break;

	case FBIOGETCMAP:

		sprintf(constring, KERN_INFO "fb_ioctl, FBIOGETCMAP\n\r");
		myprintk();

		break;

	case FBIOPUTCMAP:

		sprintf(constring, KERN_INFO "fb_ioctl, FBIOPUTCMAP\n\r");
		myprintk();

		if (copy_from_user(&cmap, (void __user *) arg, sizeof(cmap))) return -EFAULT;
		// We are not used cmaps
//		ret = fb_set_user_cmap(&cmap, info);
		break;

	}

	return ret;
}

static int am160160_fb_mmap(struct fb_info *info, struct vm_area_struct *vma){
struct fb_ops *fb;
unsigned long off;
unsigned long start;
unsigned int  len;

//	start = info->fix.smem_start;
//	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
//
//	start &= PAGE_MASK;
//	if ((vma->vm_end - vma->vm_start + off) > len) return -EINVAL;
//	off += start;
//	vma->vm_pgoff = vma->vm_start >> PAGE_SHIFT;

	sprintf(constring, KERN_INFO "fb_mmap: start 0x%X, end 0x%X, off 0x%X & len %d sets\n\r", vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_end - vma->vm_start);
	myprintk();

	// Start memory mapping
	vma->vm_ops = &am160160_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

	am160160_vma_open(vma);

	sprintf(constring, KERN_INFO "fb_mmap: memory mapping OK\n\r");
	myprintk();

	init_timer(&sync_timer);
	sync_timer.expires = jiffies + HZ/TICKSMAX;
	sync_timer.data = 0;
	sync_timer.function = sync_timer_func;
	add_timer(&sync_timer);

	return 0;
}

int am160160_fb_sync(struct fb_info *info)
{

	sprintf(constring, KERN_INFO "fb_sync\n\r");
	myprintk();

	return 0;
}

//static int am160160_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
//{
//    /* ... */
//    return 0;	   	
//}
//
//static int am160160_fb_set_par(struct fb_info *info)
//{
//    struct am160160__par *par = info->par;
//    /* ... */
//    return 0;	
//}
//
//static int am160160_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
//			   unsigned blue, unsigned transp,
//			   struct fb_info *info)
//{
//    if (regno >= 256)  /* no. of hw registers */
//       return -EINVAL;
//    /*
//     * Program hardware... do anything you want with transp
//     */
//
//    /* grayscale works only partially under directcolor */
//    if (info->var.grayscale) {
//       /* grayscale = 0.30*R + 0.59*G + 0.11*B */
//       red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
//    }
//
//    // Mono01/Mono10:
//    // Has only 2 values, black on white or white on black (fg on bg),
//    // var->{color}.offset is 0
//    /// white = (1 << var->{color}.length) - 1, black = 0
//    
//#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
//    red = CNVT_TOHW(red, info->var.red.length);
//    green = CNVT_TOHW(green, info->var.green.length);
//    blue = CNVT_TOHW(blue, info->var.blue.length);
//    transp = CNVT_TOHW(transp, info->var.transp.length);
//#undef CNVT_TOHW
//
//    if (info->fix.visual == FB_VISUAL_DIRECTCOLOR ||
//	info->fix.visual == FB_VISUAL_TRUECOLOR)
//	    write_{red|green|blue|transp}_to_clut();
//
//    if (info->fix.visual == FB_VISUAL_TRUECOLOR ||
//	info->fix.visual == FB_VISUAL_DIRECTCOLOR) {
//	    u32 v;
//
//	    if (regno >= 16)
//		    return -EINVAL;
//
//	    v = (red << info->var.red.offset) |
//		    (green << info->var.green.offset) |
//		    (blue << info->var.blue.offset) |
//		    (transp << info->var.transp.offset);
//
//	    ((u32*)(info->pseudo_palette))[regno] = v;
//    }
//
//    /* ... */
//    return 0;
//}
//
//static int am160160_fb_pan_display(struct fb_var_screeninfo *var,
//			     struct fb_info *info)
//{
//    return 0;
//}
//
//static int am160160_fb_blank(int blank_mode, struct fb_info *info)
//{
//    /* ... */
//    return 0;
//}

/* ------------ Accelerated Functions --------------------- */
//int am160160_fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
//{
//}
//void am160160_fb_rotate(struct fb_info *info, int angle)
//{
//}
//

void am160160_vma_open(struct vm_area_struct *vma){
	sprintf(constring, KERN_INFO "vma_open OK\n\r");
	myprintk();

//	(*((int*)(vma->vm_private_data)))++;
}

 void am160160_vma_close(struct vm_area_struct *vma){
	 sprintf(constring, KERN_INFO "vma_close OK\n\r");
	 myprintk();

//	 (*((int*)(vma->vm_private_data)))--;
}

int am160160_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf){
struct page *page = NULL;
void *lpage = vmf->virtual_address; /* default to "missing" */

 	  sprintf(constring, KERN_INFO "vma_fault entering\n\r");
 	  myprintk();

 	  sprintf(constring, KERN_INFO "fault (vma): start 0x%X, end 0x%X, off 0x%X\n\r", vma->vm_start, vma->vm_end, vma->vm_pgoff);
 	  myprintk();

 	  sprintf(constring, KERN_INFO "fault (vmf): virt 0x%X, off 0x%X, page 0x%X\n\r", vmf->virtual_address, vmf->pgoff, vmf->page);
 	  myprintk();

 	  mapvd = vmalloc(VID_LEN);
 	  page = vmalloc_to_page(mapvd);
 	  vmf->page = page;

 	  sprintf(constring, KERN_INFO "fault page: mapvd:0x%X, page:0x%X\n\r", mapvd, vmf->page);
 	  myprintk();

 	  /* got it, now increment the count */
      get_page(page);
      return 0;
 }

	/*
	 *  Virtual mem pages operations
	 */
static struct vm_operations_struct am160160_vm_ops = {
	.open = am160160_vma_open,
	.close = am160160_vma_close,
	.fault = am160160_vma_fault,
};

    /*
     *  Frame buffer operations
     */
static struct fb_ops am160160_fb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= am160160_fb_open,
	.fb_read	= am160160_fb_read,
	.fb_write	= am160160_fb_write,
	.fb_release	= am160160_fb_release,
//	.fb_check_var	= am160160_fb_check_var,
//	.fb_set_par	= am160160_fb_set_par,
//	.fb_setcolreg	= am160160_fb_setcolreg,
//	.fb_blank	= am160160_fb_blank,
//	.fb_pan_display	= am160160_fb_pan_display,
	.fb_fillrect	= am160160_fb_fillrect, 	/* Needed !!! */
	.fb_copyarea	= am160160_fb_copyarea,	/* Needed !!! */
	.fb_imageblit	= am160160_fb_imageblit,	/* Needed !!! */
//	.fb_cursor	= am160160_fb_cursor,		/* Optional !!! */
//	.fb_rotate	= am160160_fb_rotate,
//	.fb_sync	= am160160_fb_sync,
	.fb_ioctl	= am160160_fb_ioctl,
	.fb_mmap	= am160160_fb_mmap,
};
//EXPORT_SYMBOL_GPL(am160160_fb_read);
//EXPORT_SYMBOL_GPL(am160160_fb_write);

/* ------------------------------------------------------------------------- */

    /*
     *  Initialization
     */
/*static int __devinit am160160_fb_probe(struct pci_dev *dev,
			      const struct pci_device_id *ent)*/

static int am160160_fb_probe (struct platform_device *pdev)	// -- for platform devs
{
    struct device *dev = &pdev->dev;
    struct fb_info *info = 0;
    unsigned char *pinfo = 0;
//    struct am160160_info *aminfo;
    struct fb_info *pd_sinfo;
    struct fb_var_screeninfo *var = 0;
    struct fb_videomode *mode = 0;
    int cmap_len, retval;	
    struct list_head *head = 0;
    struct fb_modelist *list = 0;
    struct fb_event event;
    struct fb_con2fbmap con2fb;
//    unsigned int smem_len;

    am160160_device = pdev;

	am160160_resources[0] = platform_get_resource(pdev, IORESOURCE_IO, 0);
	am160160_resources[1] = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	am160160_resources[2] = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	sprintf(constring, KERN_INFO "cmd cmd 0x%X\n\r", am160160_resources[1]->start);
	myprintk();
	sprintf(constring, KERN_INFO "cmd dat 0x%X\n\r", am160160_resources[2]->start);
	myprintk();
	sprintf(constring, KERN_INFO "reset 0x%X light 0x%X\n\r", am160160_resources[0]->start, am160160_resources[0]->end);
	myprintk();
	sprintf(constring, KERN_INFO "device_open (0x%X) 0x%X 0x%X 0x%X\n\r", pdev, am160160_resources[0], am160160_resources[1], am160160_resources[2]);
	myprintk();

	/* Pins initialize */
	resetpin = am160160_resources[0]->start;
	lightpin = am160160_resources[0]->end;
	at91_set_GPIO_periph(resetpin, 0);
	at91_set_gpio_output(resetpin, 0);
	mdelay(10);
	at91_set_gpio_output(resetpin, 1);
	at91_set_GPIO_periph(lightpin, 0);
	at91_set_gpio_output(lightpin, 1);

// Registration I/O mem for indicator registers
	io_cmd = ioremap(am160160_resources[1]->start, 1);
	io_data = ioremap(am160160_resources[2]->start, BUF_LEN);
	if (!request_mem_region(io_cmd, 1, pdev->name)) return -EBUSY;
	if (!request_mem_region(io_data, BUF_LEN, pdev->name)){
		// exception 1
		release_mem_region(io_cmd, 1);
		return -EBUSY;
	}

	// Устройство описано в файле ядра board-sam9260dpm.c
	/* Hardware initialize and testing */
	// Connect to hardware driver
	if (strstr(chipname,"uc1698")) hard = uc1698_connect(io_cmd, io_data);
	if (strstr(chipname,"st7529")) hard = st7529_connect(io_cmd, io_data);
	hard->init();

    memset(video, 0, BUF_LEN);

    /*
     * Dynamically allocate info and mode
     */
    var = kmalloc(sizeof(struct fb_var_screeninfo), GFP_KERNEL);

    if (!var){
    	// exception 2
		release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
    	return -ENOMEM;
    }

    mode = kmalloc(sizeof(struct fb_videomode), GFP_KERNEL);
    if (!mode){
    	// exception 3
    	kfree(var);
		release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
    	return -ENOMEM;
    }
    memcpy(mode, &def_fb_videomode, sizeof(struct fb_videomode));

    info = framebuffer_alloc(sizeof(u32) * BUF_LEN, dev);
    if (!info){
    	// exception 4
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
    	return -ENOMEM;
    }

    if (dev->platform_data){
    	pd_sinfo = (struct fb_info *) dev->platform_data;
        memcpy(info, pd_sinfo, sizeof(struct fb_info));			// copy default fb_info to working fb_info

        sprintf(constring, KERN_INFO "Platform data exist\n\r");
    	myprintk();
    }else{
//    	pd_sinfo = &def_fb_info;

    	sprintf(constring, KERN_INFO "Platform data not exist, will default data\n\r");
    	myprintk();
    }


    mode = &def_fb_videomode;
    fb_videomode_to_var(var, mode);
    // Change videomode
    var->bits_per_pixel = 1;
    var->xres = 160;
    var->yres = 160;
    var->xres_virtual = 160;
    var->yres_virtual = 160;
    var->grayscale = 1;
    var->red.length = 1;
    var->green.length = 1;
    var->blue.length = 1;
    var->red.msb_right =1;
    var->green.msb_right =1;
    var->blue.msb_right =1;
    var->pixclock = 6500;
    var->vmode = FB_VMODE_CONUPDATE;
    var->activate = FB_ACTIVATE_ALL;
    var->sync = FB_SYNC_EXT;
    memcpy(&(info->var), var, sizeof(struct fb_var_screeninfo));

    info->fbops = &am160160_fb_ops;
    info->fix = am160160_fb_fix; /* this will be the only time am160160_fb_fix will be
     	 	 	 	 	 	 	 * used, so mark it as __devinitdata
     	 	 	 	 	 	 	 */
    info->flags = FBINFO_FLAG_DEFAULT;

    info->screen_base = video;
    info->screen_size = VID_LEN;
    info->monspecs = pd_sinfo->monspecs;
    info->par = pd_sinfo->par;

// Set videomode
    mode_option = "160x160-1@6";
    retval = fb_find_mode(&info->var, info, mode_option, mode, 1, mode, 1);
    if (!retval || retval == 4){
    	// exception 5 as 4
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
		sprintf(constring, KERN_INFO "fb not find video mode %s, ret=%d\n\r", mode_option, retval);
		myprintk();
    	return -EINVAL;
    }

//  Video Mode OK
    sprintf(constring, KERN_INFO "set fb video mode x:%d, y:%d, bpp:%d, gray:%d, xv:%d, yv:%d\n\r", info->var.xres, info->var.yres, info->var.bits_per_pixel, info->var.grayscale, info->var.xres_virtual, info->var.yres_virtual);
    myprintk();

    info->mode = mode;
    head = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    head->prev=0;
    head->next=0;

// Dinamic lets fix
//    am160160_fb_fix.smem_start = am160160_resources[2]->start;
//    am160160_fb_fix.smem_len = BUF_LEN;
//    am160160_fb_fix.mmio_start = (char __iomem *) video;
//    am160160_fb_fix.mmio_len = BUF_LEN;
    am160160_fb_fix.smem_start = video;
    am160160_fb_fix.smem_len = VID_LEN;
    am160160_fb_fix.mmio_start = io_data;
    am160160_fb_fix.mmio_len = BUF_LEN;

    memcpy(&(info->fix), &am160160_fb_fix, sizeof(struct fb_fix_screeninfo));

    info->pseudo_palette = info->par;

    sprintf(constring, KERN_INFO "set i/o sram: %lX+%lX; %lX+%lX\n\r",
    		am160160_fb_fix.mmio_start, am160160_fb_fix.mmio_len, am160160_fb_fix.smem_start, am160160_fb_fix.smem_len);
    myprintk();

    cmap_len = 16;
    if (fb_alloc_cmap(&info->cmap, cmap_len, 0) < 0){
    	// exception 6 as 5 as 4
    	kfree(head);
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
		sprintf(constring, KERN_INFO "not allocated cmap\n\r");
		myprintk();
    	return -ENOMEM;
    }

    if (register_framebuffer(info) < 0) {
    	// exception 7
    	fb_dealloc_cmap(&info->cmap);
    	kfree(head);
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
		sprintf(constring, KERN_INFO "fb not registered\n\r");
		myprintk();
    	return -EINVAL;
    }

    platform_set_drvdata(pdev, info);

    sprintf(constring, KERN_INFO "fb%d: %s frame buffer device\n\r", info->node, info->fix.id);
    myprintk();

    return 0;
}

//    /*
//     *  Cleanup
//     */

 //static void __devexit am160160_fb_remove(struct pci_dev *dev)
static int __exit am160160_fb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	sprintf(constring, KERN_INFO "fb remove\n\r");
	myprintk();

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		/* ... */
		framebuffer_release(info);
		sprintf(constring, KERN_INFO "fb removed. OK.\n\r");
		myprintk();
	}else{
		sprintf(constring, KERN_INFO "fb don't removed. False.\n\r");
		myprintk();
	}
	return 0;
}

/* for platform devices */

#ifdef CONFIG_PM
/**
 *	am160160_fb_suspend - Optional but recommended function. Suspend the device.
 *	@dev: platform device
 *	@msg: the suspend event code.
 *
 *      See Documentation/power/devices.txt for more information
 */
static int am160160_fb_suspend(struct platform_device *dev, pm_message_t msg)
{
	struct fb_info *info = platform_get_drvdata(dev);
	struct am160160_fb_par *par = info->par;

	/* suspend here */
	return 0;
}

/**
 *	am160160_fb_resume - Optional but recommended function. Resume the device.
 *	@dev: platform device
 *
 *      See Documentation/power/devices.txt for more information
 */
static int am160160_fb_resume(struct platform_dev *dev)
{
	struct fb_info *info = platform_get_drvdata(dev);
	struct am160160_fb_par *par = info->par;

	/* resume here */
	return 0;
}
#else
#define am160160_fb_suspend NULL
#define am160160_fb_resume NULL
#endif /* CONFIG_PM */

static struct platform_driver am160160_fb_driver = {
//	.probe = am160160_fb_probe,
	.remove = __exit_p(am160160_fb_remove),

#ifdef CONFIG_PM
	.suspend = am160160_fb_suspend, /* optional but recommended */
	.resume = am160160_fb_resume,   /* optional but recommended */
#endif /* CONFIG_PM */

	.driver = {
		.name = "am160160",
		.owner = THIS_MODULE,
	},
};



#ifndef MODULE
    /*
     *  Setup
     */

/*
 * Only necessary if your driver takes special options,
 * otherwise we fall back on the generic fb_setup().
 */
int __init am160160_fb_setup(char *options)
{
    /* Parse user speficied options (`video=am160160:') */
}
#endif /* MODULE */

static int __init am160160_fb_init(void)
{
	int ret;
	struct console *tcon;

	/*
	 *  For kernel boot options (in 'video=am160160_fb:<options>' format)
	 */

    if (device_cnt++) return -EINVAL;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("am160160", &option))
		return -ENODEV;
	am160160_fb_setup(option);
#endif

	// Grab tty for myprintk
    my_tty = current->signal->tty;

	am_fbmode = AMFB_CONSOLE_MODE;

	for(tcon=console_drivers; tcon != NULL; tcon = tcon->next){
		am160160fbcon = tcon;
		sprintf(constring, KERN_INFO "find console \"%s\", ptr=%lX \n\r", am160160fbcon->name, (long unsigned int) tcon);
		myprintk();
	}
	sprintf(constring, KERN_INFO "write func ptr=%lX \n\r", (long unsigned int) am160160fbcon->write);
	myprintk();

	ret = platform_driver_probe(&am160160_fb_driver, am160160_fb_probe);

	sprintf(constring, KERN_INFO "write func ptr=%lX \n\r", (long unsigned int) am160160fbcon->write);
	myprintk();

	if (ret) {
		// В случае когда девайс не добавлен
		platform_driver_unregister(&am160160_fb_driver);
		return -ENODEV;
	}

	sprintf(constring, KERN_INFO "device_open(%d)\n\r",ret);
	myprintk();

	return ret;
}

static void __exit am160160_fb_exit(void)
{
	del_timer(&sync_timer);
	platform_driver_unregister(&am160160_fb_driver);
	hard->exit();
	device_cnt--;
	sprintf(constring, KERN_INFO "device_closed\n\r");
	myprintk();
}

/* ------------------------------------------------------------------------- */
    /*
     *  Modularization
     */

module_init(am160160_fb_init);
module_exit(am160160_fb_exit);

MODULE_AUTHOR("alex AAV");
//MODULE DESCRIPTION("driver for am160160 based by uc1698 or st7529");
MODULE_SUPPORTED_DEVICE("am160160");
MODULE_LICENSE("GPL");
