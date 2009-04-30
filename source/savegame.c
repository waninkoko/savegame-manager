#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <ogcsys.h>

#include "fat.h"
#include "savegame.h"

/* Variables */
static char buffer[1024] ATTRIBUTE_ALIGN(32);


s32 __Savegame_CopyData(const char *srcpath, const char *dstpath)
{
	FILE     *infp = NULL, *outfp = NULL;
	DIR_ITER *dir  = NULL;

	char        filename[1024];
	struct stat filestat;

	s32 ret = 0;

	/* Create directory */
	mkdir(dstpath, 777);

	/* Open source directory */
	dir = diropen(srcpath);
	if (!dir)
		return -1;

	/* Get directory entries */
	while (!dirnext(dir, filename, &filestat)) {
		char inpath[128], outpath[128];

		/* Ignore invalid entries */
		if (!strcmp(filename, ".") || !strcmp(filename, ".."))
			continue;

		/* Generate paths */
		sprintf(inpath,  "%s/%s", srcpath, filename);
		sprintf(outpath, "%s/%s", dstpath, filename);

		/* Directory/File check */
		if (filestat.st_mode & S_IFDIR) {
			/* Copy directory */
			ret = __Savegame_CopyData(inpath, outpath);
			if (ret < 0)
				goto out;
		} else {
			/* Open input/output file */
			infp  = fopen(inpath,  "rb");
			outfp = fopen(outpath, "wb");
			if (!infp || !outfp) {
				ret = -1;
				goto out;
			}

			for (;;) {
				/* Read data */
				ret = fread(buffer, 1, sizeof(buffer), infp);
				if (ret < 0)
					goto out;

				/* EOF */
				if (!ret)
					break;

				/* Write data */
				ret = fwrite(buffer, 1, ret, outfp);
				if (ret < 0)
					goto out;
			}

			/* Close files */
			fclose(infp);
			fclose(outfp);
		}
	}

out:
	/* Close files */
	if (infp)
		fclose(infp);
	if (outfp)
		fclose(outfp);

	/* Close directory */
	if (dir)
		dirclose(dir);

	return ret;
}


s32 Savegame_GetNandPath(u64 tid, char *outbuf)
{
	s32 ret;

	/* Get data directory */
	ret = ES_GetDataDir(tid, buffer);
	if (ret < 0)
		return ret;

	/* Generate NAND directory */
	sprintf(outbuf, "isfs:%s", buffer);

	return 0;
}

s32 Savegame_GetTitleName(const char *path, char *outbuf)
{
	FILE *fp = NULL;

	char filepath[128];
	u16  buffer[65];
	u32  cnt;

	/* Generate filepath */
	sprintf(filepath, "%s/banner.bin", path);

	/* Open banner */
	fp = fopen(filepath, "rb");
	if (!fp)
		return -1;

	/* Read name */
	fseek(fp, 32, SEEK_SET);
	fread(buffer, sizeof(buffer), 1, fp);

	/* Close file */
	fclose(fp);

	/* Copy name */
	for (cnt = 0; buffer[cnt]; cnt++)
		outbuf[cnt] = buffer[cnt];

	outbuf[cnt] = 0;

	return 0;
}

s32 Savegame_CheckTitle(const char *path)
{
	FILE *fp = NULL;

	char filepath[128];

	/* Generate filepath */
	sprintf(filepath, "%s/banner.bin", path);

	/* Try to open banner */
	fp = fopen(filepath, "rb");
	if (!fp)
		return -1;

	/* Close file */
	fclose(fp);

	return 0;
}

s32 Savegame_Manage(u64 tid, u32 mode, const char *devpath)
{
	char nandpath[128];
	s32  ret;

	/* Set title UID */
	ret = ES_SetUID(tid);
	if (ret < 0)
		return ret;

	/* Get NAND path */
	ret = Savegame_GetNandPath(tid, nandpath);
	if (ret < 0)
		return ret;

	/* Manage savegame */
	switch (mode) {
	case SAVEGAME_EXTRACT:
		ret = __Savegame_CopyData(nandpath, devpath);
		break;

	case SAVEGAME_INSTALL:
		ret = __Savegame_CopyData(devpath, nandpath);
		break;

	default:
		return -1;
	}

	return ret;
}
