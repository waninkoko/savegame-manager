#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <ogcsys.h>

#include "fat.h"
#include "restart.h"
#include "savegame.h"
#include "title.h"
#include "usbstorage.h"
#include "utils.h"
#include "video.h"
#include "wpad.h"

/* Savegame structure */
struct savegame {
	/* Title name */
	char name[65];

	/* Title ID */
	u64 tid;
};

/* Device list */
static fatDevice deviceList[] = {
	{ "sd",		"Wii SD Slot",			&__io_wiisd },
	{ "usb",	"USB Mass Storage Device",	&__io_usbstorage },
	{ "usb2",	"USB 2.0 Mass Storage Device",	&__io_usb2storage },
	{ "gcsda",	"SD Gecko (Slot A)",		&__io_gcsda },
	{ "gcsdb",	"SD Gecko (Slot B)",		&__io_gcsdb },
};


/* Variables */
static s32 device = 0, mode = 0;

/* Macros */
#define NB_DEVICES		(sizeof(deviceList) / sizeof(fatDevice))

/* Constants */
#define ENTRIES_PER_PAGE	6
#define SAVES_DIRECTORY		"/savegames"


s32 __Menu_GetNandSaves(struct savegame **outbuf, u32 *outlen)
{
	struct savegame *buffer = NULL;

	u64 *titleList = NULL;
	u32  titleCnt;

	u32 cnt, idx;
	s32 ret;

	/* Get title list */
	ret = Title_GetList(&titleList, &titleCnt);
	if (ret < 0)
		return ret;

	/* Allocate memory */
	buffer = malloc(sizeof(struct savegame) * titleCnt);
	if (!buffer) {
		ret = -1;
		goto out;
	}

	/* Copy titles */
	for (cnt = idx = 0; idx < titleCnt; idx++) {
		u64  tid = titleList[idx];
		char savepath[128];

		/* Generate dirpath */
		Savegame_GetNandPath(tid, savepath);

		/* Check for title savegame */
		ret = Savegame_CheckTitle(savepath);
		if (!ret) {
			struct savegame *save = &buffer[cnt++];

			/* Set title name */
			Savegame_GetTitleName(savepath, save->name);

			/* Set title ID */
			save->tid = tid;
		}
	}

	/* Set values */
	*outbuf = buffer;
	*outlen = cnt;

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (titleList)
		free(titleList);

	return ret;
}

s32 __Menu_GetDeviceSaves(struct savegame **outbuf, u32 *outlen)
{
	struct savegame *buffer = NULL;
	DIR_ITER        *dir    = NULL;

	char dirpath[1024], filename[1024];
	u32  cnt = 0;

	/* Generate dirpath */
	sprintf(dirpath, "%s:" SAVES_DIRECTORY, deviceList[device].mount);

	/* Open directory */
	dir = diropen(dirpath);
	if (!dir)
		return -1;

	/* Count entries */
	for (cnt = 0; !dirnext(dir, filename, NULL); cnt++);

	/* Entries found */
	if (cnt > 0) {
		/* Allocate memory */
		buffer = malloc(sizeof(struct savegame) * cnt);
		if (!buffer) {
			dirclose(dir);
			return -2;
		}

		/* Reset directory */
		dirreset(dir);

		/* Get entries */
		for (cnt = 0; !dirnext(dir, filename, NULL);) {
			char savepath[128];
			s32  ret;

			/* Generate dirpath */
			sprintf(savepath, "%s/%s", dirpath, filename);

			/* Check for title savegame */
			ret = Savegame_CheckTitle(savepath);
			if (!ret) {
				struct savegame *save = &buffer[cnt++];

				/* Set title name */
				Savegame_GetTitleName(savepath, save->name);

				/* Set title ID */
				save->tid = StrToHex64(filename);
			}
		}
	}

	/* Close directory */
	dirclose(dir);

	/* Set values */
	*outbuf = buffer;
	*outlen = cnt;

	return 0;
}


s32 __Menu_EntryCmp(const void *p1, const void *p2)
{
	struct savegame *s1 = (struct savegame *)p1;
	struct savegame *s2 = (struct savegame *)p2;

	/* Compare entries */
	return strcmp(s1->name, s2->name);
}

s32 __Menu_RetrieveList(struct savegame **outbuf, u32 *outlen)
{
	s32 ret;

	switch (mode) {
	case SAVEGAME_EXTRACT:
		/* Retrieve from NAND */
		ret = __Menu_GetNandSaves(outbuf, outlen);
		break;

	case SAVEGAME_INSTALL:
		/* Retrieve from device */
		ret = __Menu_GetDeviceSaves(outbuf, outlen);
		break;

	default:
		return -1;
	}

	/* Sort list */
	if (ret >= 0)
		qsort(*outbuf, *outlen, sizeof(struct savegame), __Menu_EntryCmp);

	return ret;
}


void Menu_Device(void)
{
	fatDevice *dev = NULL;

	char dirpath[128];
	s32  ret;

	/* Select source device */
	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Selected device */
		dev = &deviceList[device];

		printf("\t>> Select storage device: < %s >\n\n", dev->name);

		printf("\t   Press LEFT/RIGHT to change the selected device.\n\n");

		printf("\t   Press A button to continue.\n");
		printf("\t   Press HOME button to restart.\n\n\n");

		u32 buttons = Wpad_WaitButtons();

		/* LEFT/RIGHT buttons */
		if (buttons & WPAD_BUTTON_LEFT) {
			if ((--device) <= -1)
				device = (NB_DEVICES - 1);
		}
		if (buttons & WPAD_BUTTON_RIGHT) {
			if ((++device) >= NB_DEVICES)
				device = 0;
		}

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;
	}

	printf("[+] Mounting device, please wait...");
	fflush(stdout);

	/* Mount device */
	ret = Fat_Mount(dev);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	} else
		printf(" OK!\n");

	/* Create savegames directory */
	sprintf(dirpath, "%s:" SAVES_DIRECTORY, dev->mount);
	mkdir(dirpath, 777);

	return;

err:
	/* Unmount device */
	Fat_Unmount(dev);

	printf("\n");
	printf("    Press any button to continue...\n");

	Wpad_WaitButtons();

	/* Prompt menu again */
	Menu_Device();
}

void Menu_SaveManage(struct savegame *save)
{
	fatDevice *dev = &deviceList[device];

	char devpath[128];
	s32  ret;

	printf("[+] Are you sure you want to %s this savegame?\n\n", (mode) ? "install" : "extract");

	printf("    Title Name : %s\n",          save->name);
	printf("    Title ID   : %08X-%08X\n\n", (u32)(save->tid >> 32), (u32)(save->tid & 0xFFFFFFFF));

	printf("    Press A button to continue.\n");
	printf("    Press B button to go back to the menu.\n\n\n");

	for (;;) {
		u32 buttons = Wpad_WaitButtons();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;

		/* B button */
		if (buttons & WPAD_BUTTON_B)
			return;
	}

	printf("[+] %s savegame, please wait...", (mode) ? "Installing" : "Extracting");
	fflush(stdout);

	/* Generate device/file path */
	sprintf(devpath,  "%s:" SAVES_DIRECTORY "/%016llx", dev->mount, save->tid);

	/* Manage savegame */
	ret = Savegame_Manage(save->tid, mode, devpath);
	if (ret < 0)
		printf(" ERROR! (ret = %d)\n", ret);
	else
		printf(" OK!\n");


	printf("\n");
	printf("    Press any button to continue...\n");

	Wpad_WaitButtons();
}

void Menu_SaveList(void)
{
	struct savegame *saveList = NULL;
	u32              saveCnt;

	s32 selected = 0, start = 0;
	s32 ret;

	/* Retrieve savegames */
	ret = __Menu_RetrieveList(&saveList, &saveCnt);
	if (ret < 0) {
		printf("[+] ERROR: Could not retrieve any savegames! (ret = %d)\n", ret);
		goto err;
	}

	for (;;) {
		u32 cnt;
		s32 index;

		/* Clear console */
		Con_Clear();

		/** Print entries **/
		printf("[+] Select a savegame from the list:\n\n");

		for (cnt = start; cnt < saveCnt; cnt++) {
			struct savegame *save = &saveList[cnt];

			/* Entry limit reached */
			if ((cnt - start) >= ENTRIES_PER_PAGE)
				break;

			/* Print entry */
			printf("\t%2s \"%s\"\n", (cnt == selected) ? ">>" : "  ", save->name);
		}

		printf("\n");

		printf("[+] Press A button to %s a savegame.\n", (mode) ? "install" : "extract");
		printf("    Press B button to go back to the options menu.\n");


		/** Controls **/
		u32 buttons = Wpad_WaitButtons();

		/* UP/DOWN buttons */
		if (buttons & WPAD_BUTTON_UP) {
			if ((--selected) <= -1)
				selected = (saveCnt - 1);
		}
		if (buttons & WPAD_BUTTON_DOWN) {
			if ((++selected) >= saveCnt)
				selected = 0;
		}

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			Menu_SaveManage(&saveList[selected]);

		/* B button */
		if (buttons & WPAD_BUTTON_B)
			break;


		/** Scrolling **/
		/* List scrolling */
		index = (selected - start);

		if (index >= ENTRIES_PER_PAGE)
			start += index - (ENTRIES_PER_PAGE - 1);
		if (index <= -1)
			start += index;
	}

	/* Free memory */
	free(saveList);

	return;

err:
	printf("\n");
	printf("    Press any button to continue...\n");

	Wpad_WaitButtons();
}


void Menu_Loop(void)
{
	for (;;) {
		/* Clear console */
		Con_Clear();

		printf("\t>> Select option: < %s >\n\n", (mode) ? "Install Mode" : "Extract Mode");

		printf("\t   Press LEFT/RIGHT buttons to change option.\n\n");

		printf("\t   Press A button to continue.\n");
		printf("\t   Press HOME button to restart.\n\n");

		u32 buttons = Wpad_WaitButtons();

		/* LEFT/RIGHT buttons */
		if (buttons & (WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT))
			mode ^= 1;

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* A button */
		if (buttons & WPAD_BUTTON_A) {
			/* Select device */
			Menu_Device();

			/* Show savegame lsit */
			Menu_SaveList();
		}
	}
}
