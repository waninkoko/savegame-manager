#ifndef _FAT_H_
#define _FAT_H_

/* libfat header */
#include <fat.h>

/* SD headers */
#include <sdcard/gcsd.h>
#include <sdcard/wiisd_io.h>

/* 'FAT Device' struct */
typedef struct {
	/* Device mount point */
	char *mount;

	/* Device name */
	char *name;

	/* Device interface */
	const DISC_INTERFACE *interface;
} fatDevice;

/* Prototypes */
s32   Fat_Mount(fatDevice *);
void  Fat_Unmount(fatDevice *);
char *Fat_ToFilename(const char *);

#endif
 
