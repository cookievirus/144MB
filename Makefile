# ============================================================================
# IRON & INVESTMENT - smithy demo
# Pure C99 + static Raylib. Unity build (single TU) so LTO sees everything.
# ============================================================================
CC      ?= gcc
RAYLIB  ?= ../raylib/src          # path to your trimmed raylib build

CFLAGS  = -std=c99 -Os -flto -ffunction-sections -fdata-sections \
          -fno-asynchronous-unwind-tables -fno-ident \
          -Wall -Wextra -I. -I$(RAYLIB)

ifeq ($(OS),Windows_NT)
  TARGET  = smithy.exe
  LDFLAGS = -L$(RAYLIB) -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows \
            -Wl,--gc-sections -s -flto
else
  TARGET  = smithy
  LDFLAGS = -L$(RAYLIB) -lraylib -lm -lpthread -ldl \
            -Wl,--gc-sections -s -flto
endif

all: $(TARGET)

# assets regenerate automatically when source art or maps change
assets_smithy.h: ../art/smithy.png tools/png2c.py
	python3 tools/png2c.py --colors 16 --tile 16 $< $@

map_smithy.h: ../maps/smithy.tmx tools/tmx2c.py
	python3 tools/tmx2c.py $< $@

$(TARGET): smithy_demo.c assets_smithy.h map_smithy.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	@echo "--- size budget ---"
	@stat -c "%n: %s bytes / 1474560 limit" $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
