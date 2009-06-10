#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ogcsys.h>


u64 StrToHex64(const char *str)
{
	u64 val = 0;
	u32 cnt, len;

	/* String length */
	len = strlen(str);

	for (cnt = 0; cnt < len; cnt++) {
		u32  idx = len - (cnt + 1);
		char c   = toupper(str[idx]);

		u64 n = (isdigit(c)) ? c - '0' : (c - 'A') + 0xA;
		u64 m = 1;

		for (idx = 0; idx < cnt; idx++)
			m *= 16;

		/* Convert to hex */
		val += n * m;
	}

	return val;
} 
