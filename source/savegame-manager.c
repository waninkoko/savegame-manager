#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>

#include "gui.h"
#include "isfs.h"
#include "menu.h"
#include "restart.h"
#include "sys.h"
#include "video.h"
#include "wpad.h"


int main(int argc, char **argv)
{
	s32 ret;

	/* Load Custom IOS */
	ret = IOS_ReloadIOS(249);

	/* Initialize subsystems */
	Sys_Init();

	/* Set video mode */
	Video_SetMode();

	/* Initialize console */
	Gui_InitConsole();

	/* Draw bckground */
	Gui_DrawBackground();

	/* Initialize Wiimote subsystem */
	Wpad_Init();

	/* Check for Custom IOS */
	if (ret < 0) {
		printf("[+] ERROR: Could not load Custom IOS! (ret = %d)\n", ret);
		goto out;
	}

	/* Initialize ISFS */
	ret = ISFS_Initialize();
	if (ret < 0) {
		printf("[+] ERROR: Could not initialize ISFS! (ret = %d)\n", ret);
		goto out;
	}

	/* Mount ISFS */
	ret = ISFS_Mount();
	if (ret < 0) {
		printf("[+] ERROR: Could not mount ISFS! (ret = %d)\n", ret);
		goto out;
	}

	/* Menu loop */
	Menu_Loop();

out:
	/* Restart */
	Restart_Wait();

	return 0;
}
