
CC		= arm_1103_le-gcc
LD		= arm_1103_le-ld

ASFLAGS		= -nostdinc -D__KERNEL__ -mlittle-endian -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork  -march=armv7-a -msoft-float -gdwarf-2 -Wa,-march=all
CFLAGS		= -nostdinc -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -Os -marm -fno-dwarf2-cfi-asm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -fpic -fno-builtin
AINCLUDE_PATHS	= -Iinclude -include asm/unified.h
CINCLUDE_PATHS	= -Iinclude
ZRELADDR	= 0x8000

OBJS		=

HEAD		= head.o
OBJS		+= misc.o decompress.o
OBJS		+= head-wmt.o

# Supply ZRELADDR to the decompressor via a linker symbol.
LDFLAGS_vmlinux += --defsym zreladdr=$(ZRELADDR)

# ?
LDFLAGS_vmlinux += -p
# Report unresolved symbol references
LDFLAGS_vmlinux += --no-undefined
# Delete all temporary local symbols
LDFLAGS_vmlinux += -X
# Next argument is a linker script
LDFLAGS_vmlinux += -T

# For __aeabi_uidivmod

.PHONY: all clean

all: zuboot.bin

clean :
	rm zuboot.bin vmlinux piggy.gzip *.o

%.o: %.c
	$(CC) $(CFLAGS) $(CINCLUDE_PATHS) -c -o $@ $<

%.o: %.S
	$(CC) $(ASFLAGS) $(AINCLUDE_PATHS) -c -o $@ $<

lib1funcs = lib1funcs.o

vmlinux: $(HEAD) piggy.gzip.o \
	 	$(OBJS) $(lib1funcs)
		$(LD) $(LDFLAGS) $(ldflags-y) $(LDFLAGS_vmlinux) vmlinux.lds $^ -o $@ 	 	

piggy.gzip: u-boot.bin
	cat $^ | gzip -n -f -9 > $@

piggy.gzip.o:  piggy.gzip

zuboot.bin : vmlinux
	arm_1103_le-objcopy -O binary -R .comment -S $^ $@
