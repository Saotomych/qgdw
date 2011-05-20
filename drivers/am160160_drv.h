/*
 *  Header file for AT91/AT32 LCD Controller
 *
 *  Data structure and register user interface
 *
 *  Copyright (C) 2007 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __UC1698_H__
#define __UC1698_H__

#include <linux/workqueue.h>



 /* LCD Controller info data structure, stored in device platform_data */
struct am160160_info {
	spinlock_t		lock;
	struct fb_info		*info;
	void __iomem		*mmio;
//	int			irq_base;
//	struct work_struct	task;
//	unsigned int		guard_time;
	unsigned int 		smem_len;
	struct platform_device	*pdev;

	// future for fbcon
	u8			saved_lcdcon;
	u8			default_bpp;
	u8			lcd_wiring_mode;
	unsigned int		default_lcdcon2;
	void (*u1698_fb_power_control)(int on);
	struct fb_monspecs	*default_monspecs;
	u32			pseudo_palette[16];
};

#endif /* __UC1698_H__ */
