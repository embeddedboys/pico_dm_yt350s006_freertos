# Copyright (c) 2024 embeddedboys developers
# 
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT

# LCD Pins for SPI and PIO interface
set(TFT_SPIX      0)    # which spi controller will be used.
set(TFT_SCL_PIN   18)
set(TFT_SDA_PIN   19)
set(TFT_RES_PIN   15)
set(TFT_DC_PIN    14)
set(TFT_CS_PIN    13)
set(TFT_BLK_PIN   12)
set(TFT_HOR_RES   160)
set(TFT_VER_RES   80)
set(TFT_BUS_CLK_KHZ 62500)
set(DISP_OVER_PIO 1)  # 1: PIO, 0: SPI
