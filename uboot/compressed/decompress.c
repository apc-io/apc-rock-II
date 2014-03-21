#define _LINUX_STRING_H_

#include <linux/types.h>	/* for size_t */

extern unsigned long free_mem_ptr;
extern unsigned long free_mem_end_ptr;
extern void error(char *);

#define STATIC static
#define STATIC_RW_DATA	/* non-static please */

#define ARCH_HAS_DECOMP_WDOG

#include "decompress_inflate.c"

int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x))
{
	return decompress(input, len, NULL, NULL, output, NULL, error);
}
