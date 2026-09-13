TOOLCHAIN_DIR := ../arm-gnu-toolchain-14.2.rel1-x86_64-aarch64-none-elf
TOOLCHAIN_PATH := $(shell pwd)/$(TOOLCHAIN_DIR)/bin
export PATH := $(TOOLCHAIN_PATH):$(PATH)

CC := aarch64-none-elf-gcc
LD := aarch64-none-elf-ld
OBJCOPY := aarch64-none-elf-objcopy
NM := aarch64-none-elf-nm

SRC_DIR := .
INC_DIR := include
BUILD_DIR := build
SRAM_RESTORE ?= 0

CFLAGS := -O2 -ffreestanding -nostdlib -fno-omit-frame-pointer
CFLAGS += -fno-builtin -fno-builtin-printf
CFLAGS += -mgeneral-regs-only -DWITH_NO_FP=1 -MMD -MP
CFLAGS += -DSPRIG_RESTORE_SRAM=$(SRAM_RESTORE) -I$(INC_DIR)

LDFLAGS := -T linker.ld -nostdlib
TARGET := payload.bin
ELF := $(BUILD_DIR)/payload.elf

C_SOURCES := $(shell find . -name '*.c' -not -path './build/*')
ASM_SOURCES := $(shell find . -name '*.S' -not -name 'chainload.S' -not -path './build/*')
C_OBJS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
ASM_OBJS := $(ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)
CHAINLOAD_ASM_OBJ := $(BUILD_DIR)/chainload_asm.o
OBJS := $(C_OBJS) $(ASM_OBJS) $(CHAINLOAD_ASM_OBJ)

all: $(BUILD_DIR) $(TARGET) $(BUILD_DIR)/bl2_len_offset.txt

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(TARGET)"

$(ELF): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/bl2_len_offset.txt: $(ELF)
	@va=$$($(NM) -n $< | awk '/chainload_bl2_len$$/ {print "0x"$$1; exit}'); \
	 offset=$$((va - 0x78000000)); \
	 echo "$$offset" > $@

# The payload is tiny; rebuilding avoids stale feature-flag configurations.
$(BUILD_DIR)/%.o: %.c Makefile FORCE
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile FORCE
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(CHAINLOAD_ASM_OBJ): chainload.S Makefile FORCE
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

FORCE:

.PHONY: all clean FORCE
