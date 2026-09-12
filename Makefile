# ==============================================================================
# Makefile - TRS-OS Agon Loader
# ==============================================================================

# Assembler executable (override if ez80asm is not in PATH)
# Example: make EZ80ASM=/path/to/ez80asm
EZ80ASM      ?= ez80asm
ASMFLAGS     ?=

# Paths and source files
SRC_DIR      := src
SRC          := $(SRC_DIR)/boottrs.asm
INCLUDES     := $(SRC_DIR)/ez80f92.inc
AUTOEXEC     := $(SRC_DIR)/autoexec.txt
TARGET       := boottrs.bin
LISTING      := $(SRC_DIR)/boottrs.lst
SYMBOLS      := $(SRC_DIR)/boottrs.symbols

# Upstream TRS-OS system image
SYSTEM_IMAGE := trsos.dat
UPSTREAM_ZIP := https://danielpaulmartin.com/sitepad-data/uploads/TRS-NET.zip

# MicroSD card or emulator target directory for deployment
SDCARD       ?=

.PHONY: all clean listing fetch install

all: $(TARGET)

$(TARGET): $(SRC) $(INCLUDES)
	@command -v $(EZ80ASM) > /dev/null 2>&1 || { \
		echo "Error: '$(EZ80ASM)' assembler not found in PATH."; \
		echo "Please install ez80asm or specify its location, for example:"; \
		echo "  make EZ80ASM=/path/to/ez80asm"; \
		exit 1; \
	}
	(cd $(SRC_DIR) && $(EZ80ASM) $(ASMFLAGS) boottrs.asm ../$(TARGET))

listing: ASMFLAGS += -l
listing: $(TARGET)

# Fetch the upstream TRS-OS 480 KB image from Daniel Paul Martin's site
fetch: $(SYSTEM_IMAGE)

$(SYSTEM_IMAGE):
	@echo "Fetching TRS-OS system image from upstream..."
	curl -s -L -o TRS-NET.zip $(UPSTREAM_ZIP)
	unzip -p TRS-NET.zip TRS-OS_Squirrel.bin > $(SYSTEM_IMAGE)
	rm -f TRS-NET.zip
	@echo "Downloaded $(SYSTEM_IMAGE) successfully."

# Deploy boot files to a microSD card or emulator directory
install: $(TARGET) $(SYSTEM_IMAGE)
	@if [ -z "$(SDCARD)" ]; then \
		echo "Error: SDCARD destination directory not specified."; \
		echo "Usage: make install SDCARD=/path/to/sdcard"; \
		exit 1; \
	fi
	@mkdir -p $(SDCARD)
	cp $(TARGET) $(SYSTEM_IMAGE) $(SDCARD)/
	cp $(AUTOEXEC) $(SDCARD)/autoexec.txt
	@echo "Deployed $(TARGET), $(SYSTEM_IMAGE), and autoexec.txt to $(SDCARD)/"

clean:
	rm -f $(TARGET) $(LISTING) $(SYMBOLS)
