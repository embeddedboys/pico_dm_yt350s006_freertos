# Copyright (c) 2024 embeddedboys developers
# 
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT

# LCD Pins for SPI and PIO interface
set(TFT_SPIX      0)    # which spi controller will be used.
set(TFT_SCL_PIN   10)
set(TFT_SDA_PIN   11)
set(TFT_RES_PIN   15)
set(TFT_DC_PIN    14)
set(TFT_CS_PIN    9)
set(TFT_BLK_PIN   13)
set(TFT_HOR_RES   240)
set(TFT_VER_RES   320)
set(TFT_BUS_CLK_KHZ 62500)
set(DISP_OVER_PIO 1)  # 1: PIO, 0: SPI

# Panel Model selection
set(TFT_MODEL_P169H002  0)
set(TFT_MODEL_YT280S030 1)

add_definitions(-DTFT_MODEL_P169H002=${TFT_MODEL_P169H002})
add_definitions(-DTFT_MODEL_YT280S030=${TFT_MODEL_YT280S030})
