#ifndef _UTILS_H_
#define _UTILS_H_

/* Macros */
#define round_up(x,n)	(-(-(x) & -(n)))

/* Prototypes */
u64 StrToHex64(const char *); 

#endif
