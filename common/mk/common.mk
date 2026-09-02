# common.mk - regles de build partagees pour tous les TP (NUCLEO-F303K8)
#
# Un Makefile de TP doit definir, avant d'inclure ce fichier :
#   TARGET  = nom du binaire (sans extension)
#   SRCS    = liste des fichiers .c du TP (chemins relatifs au dossier du TP)
#   INCDIRS = (optionnel) dossiers d'include supplementaires du TP
#
# Puis faire : include ../common/mk/common.mk

# Racine du repo, calculee depuis l'emplacement de CE fichier (robuste
# quel que soit le TP qui inclut common.mk)
MK_DIR    := $(dir $(lastword $(MAKEFILE_LIST)))
ROOT_DIR  := $(abspath $(MK_DIR)/../..)

# --- Toolchain ---
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc -x assembler-with-cpp
LD      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size
GDB     = gdb-multiarch

# --- Cible : Cortex-M4F avec FPU simple precision (le F103RB du cours n'a pas de FPU) ---
CPU      = -mcpu=cortex-m4
FPU      = -mfpu=fpv4-sp-d16
FLOATABI = -mfloat-abi=hard
MCUFLAGS = $(CPU) -mthumb $(FPU) $(FLOATABI)

# --- Sources et includes communs (CMSIS + startup) ---
CMSIS_CORE   = $(ROOT_DIR)/vendor/cmsis/core
CMSIS_DEVICE = $(ROOT_DIR)/vendor/cmsis/device
STARTUP_SRC  = $(ROOT_DIR)/vendor/startup/startup_stm32f303x8.s
LDSCRIPT     = $(ROOT_DIR)/common/linker/STM32F303K8Tx_FLASH.ld
OPENOCD_CFG  = $(ROOT_DIR)/tools/openocd_f303k8.cfg

COMMON_SRCS = $(CMSIS_DEVICE)/system_stm32f3xx.c $(STARTUP_SRC)

DEFS = -DSTM32F303x8

INCLUDES = -I$(CMSIS_CORE) -I$(CMSIS_DEVICE) $(addprefix -I,$(INCDIRS))

# --- Flags ---
CFLAGS  = $(MCUFLAGS) $(DEFS) $(INCLUDES)
CFLAGS += -std=c11 -Wall -Wextra -Og -g3 -ffunction-sections -fdata-sections
ASFLAGS = $(MCUFLAGS) -g3
LDFLAGS = $(MCUFLAGS) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

BUILD_DIR = build

ALL_SRCS = $(SRCS) $(COMMON_SRCS)
OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(ALL_SRCS:.c=.o)))
OBJS := $(OBJS:.s=.o)

vpath %.c $(sort $(dir $(ALL_SRCS)))
vpath %.s $(sort $(dir $(ALL_SRCS)))

.PHONY: all flash debug openocd clean size

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex size

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

# --- Flash via openocd (ST-LINK) ---
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f $(OPENOCD_CFG) -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# --- Debug : lance openocd en arriere-plan puis gdb-multiarch attache dessus ---
# (openocd ouvre un serveur gdb sur le port 3333)
openocd:
	openocd -f $(OPENOCD_CFG)

debug: $(BUILD_DIR)/$(TARGET).elf
	openocd -f $(OPENOCD_CFG) & \
	OPENOCD_PID=$$!; \
	trap "kill $$OPENOCD_PID 2>/dev/null" EXIT; \
	sleep 1; \
	$(GDB) -ex "target extended-remote localhost:3333" $(BUILD_DIR)/$(TARGET).elf; \
	kill $$OPENOCD_PID 2>/dev/null

clean:
	rm -rf $(BUILD_DIR)
