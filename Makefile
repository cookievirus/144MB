# IRON & INVESTMENT - Demo 1.1
RAYLIB ?= /usr/local
CC     ?= gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -DPLATFORM_DESKTOP_RGFW -I$(RAYLIB)/include -Isrc
LDFLAGS = -L$(RAYLIB)/lib -lraylib -lm -lpthread -ldl -lrt -lX11

SRC = src/main.c src/gfx.c src/rarity.c src/ui_font.c src/ui_dialog.c \
      src/ui_menu.c src/ui_prompt.c src/scene_smithy.c
HDR = assets/bg_smithy.h assets/hero_idle.h assets/portraits.h assets/font5x7.h

iron_demo: $(SRC) $(HDR)
	$(CC) $(CFLAGS) src/main.c -o $@ $(LDFLAGS)
	strip $@
	@ls -l $@ | awk '{printf "Binary size: %s bytes (budget 1474560)\n", $$5}'

assets:
	python3 tools/png2c.py BG-01-SMITTY-A03.png assets/bg_smithy.h bg_smithy \
	    --w 320 --h 240 --colors 32 --denoise 5
	python3 tools/png2c.py HERO-BEST-07.png assets/hero_idle.h hero_idle \
	    --h 184 --colors 16 --crop --alpha-cut 0.55
	@mkdir -p build
	python3 tools/make_portraits.py HERO-BEST-MOOD-01.png build/portraits.png
	python3 tools/png2c.py build/portraits.png assets/portraits.h portraits --colors 16
	python3 tools/font2c.py assets/font5x7.h

clean:
	rm -f iron_demo

.PHONY: assets clean
