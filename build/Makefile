# ZeroOs Makefile
# Phase 2: Kernel core with interrupts, memory management, scheduling

NASM = nasm
CC = gcc
LD = ld

# Flags for freestanding kernel
CFLAGS = -ffreestanding -fno-stack-protector -nostdlib -nostdinc \
         -m64 -mcmodel=large -I../include -Wall -Wextra -c
LDFLAGS = -T ../linker/kernel.ld -nostdlib

# Directories
BUILD_DIR = .
BOOT_DIR = ../boot
KERNEL_DIR = ../kernel
INCLUDE_DIR = ../include

# Source files
BOOT_STAGE1 = $(BOOT_DIR)/boot.asm
BOOT_STAGE2 = $(BOOT_DIR)/stage2.asm
KERNEL_ASM = $(KERNEL_DIR)/entry.asm $(KERNEL_DIR)/isr.asm
KERNEL_SRC = $(KERNEL_DIR)/kmain.c $(KERNEL_DIR)/vga.c $(KERNEL_DIR)/idt.c \
             $(KERNEL_DIR)/timer.c $(KERNEL_DIR)/keyboard.c \
             $(KERNEL_DIR)/mm.c $(KERNEL_DIR)/vmm.c $(KERNEL_DIR)/heap.c \
             $(KERNEL_DIR)/process.c $(KERNEL_DIR)/syscall.c \
             $(KERNEL_DIR)/string.c

# Output files
BOOT1_BIN = $(BUILD_DIR)/boot.bin
BOOT2_BIN = $(BUILD_DIR)/stage2.bin
KERNEL_ASM_OBJS = $(BUILD_DIR)/entry.o $(BUILD_DIR)/isr.o
KERNEL_C_OBJS = $(BUILD_DIR)/kmain.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/idt.o \
                $(BUILD_DIR)/timer.o $(BUILD_DIR)/keyboard.o \
                $(BUILD_DIR)/mm.o $(BUILD_DIR)/vmm.o $(BUILD_DIR)/heap.o \
                $(BUILD_DIR)/process.o $(BUILD_DIR)/syscall.o \
                $(BUILD_DIR)/string.o
KERNEL_OBJS = $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
DISK_IMG = $(BUILD_DIR)/disk.img

.PHONY: all clean run debug

all: $(DISK_IMG)

# Stage 1 bootloader (512 bytes, MBR)
$(BOOT1_BIN): $(BOOT_STAGE1)
	$(NASM) -f bin $< -o $@

# Stage 2 bootloader (64 sectors = 32KB)
$(BOOT2_BIN): $(BOOT_STAGE2)
	$(NASM) -f bin $< -o $@

# Kernel assembly sources (entry, ISR stubs)
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	$(NASM) -f elf64 $< -o $@

# Kernel C sources
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

# Link kernel (ELF)
$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Convert ELF to flat binary (raw) for bootloader
$(KERNEL_BIN): $(BUILD_DIR)/kernel.elf
	objcopy -O binary $< $@

# Create disk image
$(DISK_IMG): $(BOOT1_BIN) $(BOOT2_BIN) $(KERNEL_BIN)
	@echo "Creating disk image..."
	@# Create a 1.44MB floppy image
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	@# Write Stage 1 bootloader to MBR (first 512 bytes)
	dd if=$(BOOT1_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	@# Write Stage 2 bootloader starting at sector 2
	dd if=$(BOOT2_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@# Write kernel starting at sector 66 (after MBR + 64 sectors of stage2)
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=65 conv=notrunc 2>/dev/null
	@echo "Disk image created: $@"

# Run in QEMU
run: $(DISK_IMG)
	qemu-system-x86_64 \
		-drive file=$(DISK_IMG),format=raw,index=0,media=disk \
		-serial stdio \
		-m 256M \
		-no-reboot \
		-display default

# Run with serial debug output only
debug: $(DISK_IMG)
	qemu-system-x86_64 \
		-drive file=$(DISK_IMG),format=raw,index=0,media=disk \
		-serial stdio \
		-display none \
		-m 256M \
		-no-reboot

# Clean build artifacts
clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.o $(BUILD_DIR)/*.elf $(BUILD_DIR)/*.img
