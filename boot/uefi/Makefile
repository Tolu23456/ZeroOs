# ZeroOs UEFI Bootloader Makefile (GNU-EFI)

CC = gcc
OBJCOPY = objcopy
LD = ld

# GNU-EFI paths
GNU_EFI = /usr/lib/gnu-efi
GNU_EFI_INC = /usr/include/efi
GNU_EFI_LIB = /usr/lib/gnu-efi

# Compiler flags
CFLAGS = -ffreestanding -fno-stack-protector -nostdlib -nostdinc \
         -m64 -mcmodel=small -I. -I$(GNU_EFI_INC) \
         -I$(GNU_EFI_INC)/x86_64 -I$(GNU_EFI_INC)/x86_64/protocol \
         -Wall -Wextra -c -DEFI_FUNCTION_WRAPPER

# Linker flags for PE/COFF
LDFLAGS = -shared -Bsymbolic \
    -L$(GNU_EFI_LIB)/x86_64 \
    -e efi_main \
    -subsystem 10 \
    --oformat pei-x86-64

# Libraries
LIBS = -lgnuefi -lefi

# Source files
SRC = boot.c
OBJ = boot.o

# Output
BOOTLOADER = BOOTX64.EFI

.PHONY: all clean

all: $(BOOTLOADER)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) $< -o $@

$(BOOTLOADER): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ) $(BOOTLOADER)
