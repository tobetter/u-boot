/*
* (C) Copyright 2014 Hardkernel Co,.Ltd
*
* See file CREDITS for list of people who contributed to this
* project.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston,
* MA 02111-1307 USA
*/

#include <common.h>
#include <video_fb.h>
#include <asm/arch/osd.h>
#include <asm/arch/gpio.h>
#include <edid.h>

#include <amlogic/aml_tv.h>

#define HDMI_HPD		GPIOH_0

#if CONFIG_AML_HDMI_TX
void hdmi_tx_power_init(void)
{
}

int hdmi_isconnected()
{
	return amlogic_get_value(HDMI_HPD);
}

struct vmode {
	int xres;
	int yres;
	int vfreq;
	int xwin;
	int ywin;
	char *mode;
	int tvout;
};

static struct vmode *vmode = NULL;

static struct vmode vmodes[] = {
	{ 1920, 1080, 60, 1920, 1080, "1080p", TVOUT_1080P },
	{ 1920, 1200, 60, 1920, 1200, "1920x1200", TVOUT_1920X1200 },
	{ 1680, 1050, 60, 1920, 1080, "1680x1050p60hz", TVOUT_1680X1050P_60HZ },
	{ 1600,  900, 60, 1600,  900, "1600x900p60hz", TVOUT_1600X900P_60HZ },
	{ 1440,  900, 60, 1440,  900, "1440x900p60hz", TVOUT_1440X900P_60HZ },
	{ 1360,  768, 60, 1920, 1080, "1360x768p60hz", TVOUT_1360X768P_60HZ },
	{ 1280, 1024, 60, 1280, 1024, "1280x1024p60hz", TVOUT_1280X1024P_60HZ },
	{ 1280,  800, 60, 1920, 1080, "1280x800p60hz", TVOUT_1280X1024P_60HZ },
	{ 1280,  720, 60, 1280,  720, "720p", TVOUT_720P },
	{ 1024,  768, 60, 1024,  768, "1024x768p60hz", TVOUT_1024X768P_60HZ },
	{ 1024,  600, 60, 1024,  600, "1024x600p60hz", TVOUT_1024X600P_60HZ },
	{  800,  600, 60,  800,  600, "800x600p60hz", TVOUT_800X600P_60HZ },
	{  800,  480, 60,  800,  480, "800x480p60hz", TVOUT_800X480P_60HZ },
	{  640,  480, 60,  640,  480, "640x480p60hz", TVOUT_640X480P_60HZ },
};

static struct vmode* get_best_vmode(int xres, int yres, int vfreq)
{
	int i;
	struct vmode *res;

	for (i = 0; i < sizeof(vmodes) / sizeof(vmodes[0]); i++) {
		res = &vmodes[i];
		if ((res->xres == xres) && (res->yres == yres) &&
				(res->vfreq == vfreq)) {
			return res;
		}
	}

	return NULL;
}

#if defined(CONFIG_I2C_EDID)
static struct edid1_info edid;

static char* outputmode_by_edid(void)
{
	int i;
	struct vmode *vmode;

        puts("VIDEO: ");
	if (i2c_read(0x50, 0, 1, (uchar *)&edid, sizeof(edid)) != 0) {
		puts("Error reading EDID content.\n");
		return NULL;
	}

	if (edid_check_info(&edid)) {
		puts("Content isn't valid EDID.\n");
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(edid.standard_timings); i++) {
		unsigned int aspect = 10000;
		unsigned int x, y;
		unsigned char xres, vfreq;

		xres = EDID1_INFO_STANDARD_TIMING_XRESOLUTION(edid, i);
		vfreq = EDID1_INFO_STANDARD_TIMING_VFREQ(edid, i);
		if ((xres != vfreq) ||
		    ((xres != 0) && (xres != 1)) ||
		    ((vfreq != 0) && (vfreq != 1))) {
			switch (EDID1_INFO_STANDARD_TIMING_ASPECT(edid, i)) {
			case ASPECT_625:
				aspect = 6250;
				break;
			case ASPECT_75:
				aspect = 7500;
				break;
			case ASPECT_8:
				aspect = 8000;
				break;
			case ASPECT_5625:
				aspect = 5625;
				break;
			}
			x = (xres + 31) * 8;
			y = x * aspect / 10000;
			vmode = get_best_vmode(x, y, (vfreq & 0x3f) + 60);
			if (vmode) {
                                printf("%dx%d%c %d Hz is detected\n", x, y,
                                                x > 1000 ? ' ' : '\t', (vfreq & 0x3f) + 60);
				return vmode;
                        }
		}
	}

	return NULL;
}
#endif /* CONFIG_I2C_EDID */

int board_graphic_device(GraphicDevice *dev)
{
	amlogic_gpio_direction_input(HDMI_HPD);

	if (hdmi_isconnected()) {
#if defined(CONFIG_I2C_EDID)
		vmode = outputmode_by_edid();
#endif
	}

	if (vmode == NULL)
		return 0;

	dev->frameAdrs = 0x07900000;
	dev->fb_width = vmode->xres;
	dev->fb_height = vmode->yres;
	dev->winSizeX = vmode->xwin;
	dev->winSizeY = vmode->ywin;;
	dev->gdfIndex = GDF_32BIT_X888RGB;
        dev->gdfBytesPP = 4;
	dev->fg = 0xffff;
	dev->bg = 0x0000;

	return vmode->tvout;
}

int board_video_skip()
{
        int splash;

        /* Video init could be skipped if HDMI cable is not attached on boot */
	if (!hdmi_isconnected())
                return 1;

        /* Read splash image from FAT partiton, it must be 'bootsplash.bmp' as
         * a file. If failed try to read from raw sectors of 'logo' partition.
         */
        splash = run_command("fatload mmc 0:1 ${loadaddr_logo} bootsplash.bmp", 0);
        if (splash != 0) {
                /* Load boot splash bitmap image from storage */
                splash = run_command("movi read logo 0 ${loadaddr_logo}", 0);
        }

        /* Splash image is loaded to memory */
        if (splash == 0) {
                setenv("splashimage", getenv("loadaddr_logo"));

#ifdef CONFIG_SPLASH_SCREEN_ALIGN
                /* Let position the splash image at center of display */
                setenv("splashpos", "m,m");
#endif
        }

        return 0;
}
#endif

#if defined(CONFIG_CONSOLE_EXTRA_INFO)
void video_get_info_str(int line_number, char *info)
{
        *info = NULL;

        switch (line_number) {
        case 1: sprintf(info, " BOARD: ODROID-C1");
                break;
        case 2: sprintf(info, " S/N  : %s", getenv("fbt_id#"));
                break;
        case 3: sprintf(info, " MAC  : %s", getenv("ethaddr"));
                break;
        case 4: if (vmode) {
                        sprintf(info, " VIDEO: %dx%d %dHz",
                                        vmode->xres, vmode->yres, vmode->vfreq);
                }
        }
}
#endif
