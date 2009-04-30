#ifndef _SAVEGAME_H_
#define _SAVEGAME_H_

/* Constants */
#define SAVEGAME_EXTRACT	0
#define SAVEGAME_INSTALL	1

/* Prototypes */
s32 Savegame_GetNandPath(u64, char *);
s32 Savegame_GetTitleName(const char *, char *);
s32 Savegame_CheckTitle(const char *);
s32 Savegame_Manage(u64, u32, const char *);

#endif
