# IRON & INVESTMENT - Demo 1.9.3
RAYLIB ?= /usr/local
CC     ?= gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -DPLATFORM_DESKTOP_RGFW -I$(RAYLIB)/include -Isrc
LDFLAGS = -L$(RAYLIB)/lib -lraylib -lm -lpthread -ldl -lrt -lX11

SRC = src/main.c src/gfx.c src/rarity.c src/ui_font.c src/ui.c \
      src/sort.c src/title.c src/inventory.c src/shop.c src/vfx.c src/qte.c src/forge.c src/ui_dialog.c \
      src/ui_menu.c src/ui_prompt.c src/scene.c
HDR = assets/bg_smithy.h assets/bg_shop.h assets/hero_idle.h \
      assets/merchant_idle.h assets/portraits.h \
      assets/portraits_merchant.h assets/font5x7.h

iron_demo: $(SRC) $(HDR)
	$(CC) $(CFLAGS) src/main.c -o $@ $(LDFLAGS)
	strip $@
	@ls -l $@ | awk '{printf "Binary size: %s bytes (budget 1474560)\n", $$5}'

assets: check
	python3 tools/png2c.py resource/BG-01-SMITTY-A03.png assets/bg_smithy.h bg_smithy \
	    --w 320 --h 240 --colors 32 --denoise 5
	python3 tools/png2c.py resource/BG-01-ITEM_SHOP-A01.png assets/bg_shop.h bg_shop \
	    --w 320 --h 240 --colors 48 --denoise 0
	python3 tools/png2c.py resource/HERO-BEST-07.png assets/hero_idle.h hero_idle \
	    --h 184 --colors 16 --crop --alpha-cut 0.55
	@mkdir -p build
	python3 tools/make_portraits.py resource/HERO-BEST-MOOD-01.png build/portraits.png
	python3 tools/png2c.py build/portraits.png assets/portraits.h portraits --colors 16
	python3 tools/png2c.py resource/NPC-JACK-SIZE_614x819-04.png \
	    assets/merchant_idle.h merchant_idle --h 168 --colors 16 --crop --alpha-cut 0.55
	python3 tools/make_portraits.py resource/MERCHANT-MOOD-01.png \
	    build/portraits_merchant.png --shift 26,10
	python3 tools/png2c.py build/portraits_merchant.png \
	    assets/portraits_merchant.h portraits_merchant --colors 16
	python3 tools/font2c.py assets/font5x7.h

# Source art is validated before it is converted, so a bad export fails at
# the tool rather than as a sliver or a halo on screen.
check:
	python3 tools/check_asset.py bg   resource/BG-01-SMITTY-A03.png
	python3 tools/check_asset.py bg   resource/BG-01-ITEM_SHOP-A01.png
	python3 tools/check_asset.py char resource/HERO-BEST-07.png
	python3 tools/check_asset.py mood resource/HERO-BEST-MOOD-01.png
	python3 tools/check_asset.py char resource/NPC-JACK-SIZE_614x819-04.png
	python3 tools/check_asset.py mood resource/MERCHANT-MOOD-01.png

# Headless logic tests under ASan + UBSan. No window, no GPU.
TESTS = test_shop test_entrance test_sort test_title test_forge test_qte test_day test_vfx
test:
	@mkdir -p build
	@for t in $(TESTS); do \
	  echo "== $$t"; \
	  $(CC) -std=c11 -O1 -g -fsanitize=address,undefined -I$(RAYLIB)/include -Isrc \
	    -o build/$$t tests/$$t.c tests/raylib_stub.c -lm || exit 1; \
	  ./build/$$t | tail -1 || exit 1; \
	done

clean:
	rm -f iron_demo
	rm -rf build

.PHONY: assets check test clean
