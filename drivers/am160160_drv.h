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

#define 	off					0

// UC1698 Command set
#define 	SETCOLADDR_L		0x00
#define 	SETCOLADDR_H		0x10
#define 	SETTEMPCOMP			0x24
#define		SETPWRCTL			0x28
#define 	SETPRGCTL_2B		0x30
#define		SETSCRLN_L			0x40
#define 	SETSCRLN_H			0x50
#define		SETROWADDR_L		0x60
#define		SETROWADDR_H		0x70
#define		SETV_2B				0x81
#define		SETPARTCTL			0x84
#define		SETRAMCTL			0x88
#define		SETFIXLN_2B			0x90
#define		SETLNRATE			0xA0
#define		SETALLPXON			0xA4
#define 	SETALLPXINV			0xA6
#define		SETDISPEN			0xA8
#define		SETLCDMAP			0xC0
#define		SETNLNINV_2B		0xC8
#define		SETCOLPAT			0xD0
#define		SETCOLMOD			0xD4
#define		SETCOMSCAN			0xD8
#define		RESET				0xE2
#define		NOP					0xE3
#define		SETTESTCTL_2B		0xE4
#define		SETLCDBIASRT		0xE8
#define		SETCOMEND_2B		0xF1
#define		SETPARTSTART_2B		0xF2
#define		SETPARTEND_2B		0xF3
#define		SETWINCOLSTART_2B	0xF4
#define		SETWINROWSTART_2B	0xF5
#define		SETWINCOLEND_2B		0xF6
#define		SETWINROWEND_2B		0xF7
#define		WINPRGMOD			0xF8
#define		SETMTPOPS_2B		0xB8
#define		SETMTPWRMASK_3B		0xB9
#define		SETV1_2B			0xB4
#define		SETV2_2B			0xB5
#define		SETMTPWRTMR_2B		0xF6
#define		SETMTPRDTMR_2B		0xF7



 /* LCD Controller info data structure, stored in device platform_data */
struct uc1698_info {
	spinlock_t		lock;
	struct fb_info		*info;
	void __iomem		*mmio;
	int			irq_base;
	struct work_struct	task;

	unsigned int		guard_time;
	unsigned int 		smem_len;
	struct platform_device	*pdev;
	struct clk		*bus_clk;
	struct clk		*lcdc_clk;

#ifdef CONFIG_BACKLIGHT_ATMEL_LCDC
	struct backlight_device	*backlight;
	u8			bl_power;
#endif
	bool			lcdcon_is_backlight;
	u8			saved_lcdcon;

	u8			default_bpp;
	u8			lcd_wiring_mode;
	unsigned int		default_lcdcon2;
	unsigned int		default_dmacon;
	void (*u1698_fb_power_control)(int on);
	struct fb_monspecs	*default_monspecs;
	u32			pseudo_palette[16];
};

#endif /* __UC1698_H__ */
