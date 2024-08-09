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
set(TFT_HOR_RES   480)
set(TFT_VER_RES   320)
if(${SYS_CLK_KHZ} GREATER_EQUAL 400000)
    set(TFT_BUS_CLK_KHZ 100000) # 100MHz Maximum
elseif(${SYS_CLK_KHZ} GREATER_EQUAL 360000 AND ${SYS_CLK_KHZ} LESS 400000)
    set(TFT_BUS_CLK_KHZ 62500)
else()
set(TFT_BUS_CLK_KHZ 50000)
endif()
message(STATUS "TFT_BUS_CLK_KHZ=${TFT_BUS_CLK_KHZ}")
set(DISP_OVER_PIO 1)  # 1: PIO, 0: SPI

# Panel Model selection
set(TFT_MODEL_YT350S006 0)
set(TFT_MODEL_HP35006_D 1)
add_definitions(-DTFT_MODEL_YT350S006=${TFT_MODEL_YT350S006})
add_definitions(-DTFT_MODEL_HP35006_D=${TFT_MODEL_HP35006_D})
