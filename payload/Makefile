# Modified by 秋逸(逸) <2898684403@qq.com>
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

CFLAGS := -O2 -ffreestanding -nostdlib -fno-omit-frame-pointer
CFLAGS += -fno-builtin -fno-builtin-printf
CFLAGS += -mgeneral-regs-only -DWITH_NO_FP=1
CFLAGS += -I$(INC_DIR)

LDFLAGS := -T linker.ld -nostdlib

TARGET := payload.bin
ELF := $(BUILD_DIR)/payload.elf

C_SOURCES := $(shell find . -name '*.c' -not -path './build/*')
ASM_SOURCES := $(shell find . -name '*.S' -not -name 'chainload.S' -not -path './build/*')

C_OBJS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
ASM_OBJS := $(ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)
OBJS := $(C_OBJS) $(ASM_OBJS)

# chainload.S shares its basename with chainload.c, which shadows the
# pattern rule; give it an explicit, distinct object name.
CHAINLOAD_ASM_OBJ := $(BUILD_DIR)/chainload_asm.o
OBJS += $(CHAINLOAD_ASM_OBJ)

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
	 offset=$$((va - 0xB8000000)); \
	 echo "$$offset" > $@; \
	 echo "bl2_len offset: $$offset"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(CHAINLOAD_ASM_OBJ): chainload.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
