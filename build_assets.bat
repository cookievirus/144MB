@echo off
REM Regenerates assets\*.h from the source art. Build-time only.

REM Validate first: a bad export fails here instead of on screen.
python tools\check_asset.py bg   resource\BG-01-SMITTY-A03.png     || goto :err
python tools\check_asset.py bg   resource\BG-01-ITEM_SHOP-A01.png  || goto :err
python tools\check_asset.py char resource\HERO-BEST-07.png         || goto :err
python tools\check_asset.py mood resource\HERO-BEST-MOOD-01.png    || goto :err
python tools\check_asset.py char resource\NPC-JACK-SIZE_614x819-04.png || goto :err
python tools\check_asset.py mood resource\MERCHANT-MOOD-01.png    || goto :err

python tools\png2c.py resource\BG-01-SMITTY-A03.png assets\bg_smithy.h bg_smithy ^
    --w 320 --h 240 --colors 32 --denoise 5
python tools\png2c.py resource\BG-01-ITEM_SHOP-A01.png assets\bg_shop.h bg_shop ^
    --w 320 --h 240 --colors 48 --denoise 0
python tools\png2c.py resource\HERO-BEST-07.png assets\hero_idle.h hero_idle ^
    --h 184 --colors 16 --crop --alpha-cut 0.55
if not exist build mkdir build
python tools\make_portraits.py resource\HERO-BEST-MOOD-01.png build\portraits.png
python tools\png2c.py build\portraits.png assets\portraits.h portraits --colors 16
python tools\font2c.py assets\font5x7.h

REM JACK. --h 168 against BEST's 184 is template-matched against
REM REF-SHOP-COMPOSITE-02.png, not chosen by eye. --shift moves the portrait
REM square off his hood and onto his face.
python tools\png2c.py resource\NPC-JACK-SIZE_614x819-04.png ^
    assets\merchant_idle.h merchant_idle --h 168 --colors 16 --crop --alpha-cut 0.55
python tools\make_portraits.py resource\MERCHANT-MOOD-01.png ^
    build\portraits_merchant.png --shift 26,10
python tools\png2c.py build\portraits_merchant.png assets\portraits_merchant.h ^
    portraits_merchant --colors 16
goto :eof

:err
echo.
echo Asset validation failed - nothing was regenerated.
