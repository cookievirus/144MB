@echo off
REM Regenerates assets\*.h from the source art. Build-time only.
python tools\png2c.py BG-01-SMITTY-A03.png assets\bg_smithy.h bg_smithy ^
    --w 320 --h 240 --colors 32 --denoise 5
python tools\png2c.py HERO-BEST-07.png assets\hero_idle.h hero_idle ^
    --h 184 --colors 16 --crop --alpha-cut 0.55
python tools\make_portraits.py HERO-BEST-MOOD-01.png build\portraits.png
python tools\png2c.py build\portraits.png assets\portraits.h portraits --colors 16
python tools\font2c.py assets\font5x7.h
