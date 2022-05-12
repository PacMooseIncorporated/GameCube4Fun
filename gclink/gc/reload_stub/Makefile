DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

OPTS	= -I./include -ffast-math -flto -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections -fno-zero-initialized-in-bss -Wl,--gc-sections -Ibase -Wno-address-of-packed-member -Wno-scalar-storage-order

DEST    = ../gclink/source

DISASM    = disassembly

.NOTPARALLEL:
all: clean stub

stub: stub.bin

clean:
	@rm -f *.bin *.elf *.o
	@rm -f $(DEST)/*.s
	@rm -fr $(DISASM)

stub.bin:
	@echo Building Stub ...
	@$(CC) -Os $(OPTS) -c stub/crt0.S
	@$(CC) -Os $(OPTS) -c stub/main.c
	@$(CC) -Os $(OPTS) -c stub/asmfunc.S
	@$(CC) -Os $(OPTS) -c stub/mmcbbp.c
	@$(CC) -Os $(OPTS) -c stub/pff.c
	@$(CC) -Os $(OPTS) -c stub/os.c
	@$(CC) -Os $(OPTS) -o stub.elf -T stub/stub.ld crt0.o main.o asmfunc.o mmcbbp.o pff.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D stub.elf > $(DISASM)/stub.txt
	@$(OBJCOPY) -O binary stub.elf stub.bin
	@$(BIN2S) stub.bin > $(DEST)/Stub.s
	@rm -f *.bin *.elf *.o