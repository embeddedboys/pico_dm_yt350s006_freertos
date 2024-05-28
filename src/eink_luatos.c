// Copyright (c) 2024 embeddedboys developers
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "tft.h"

#if LCD_DRV_USE_EINK_LUATOS

enum {
    EINK_LUATOS_REFRESH_PART,
    EINK_LUATOS_REFRESH_FULL,
};

static const unsigned char EPD_1IN54_lut_full_update[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

static const unsigned char EPD_1IN54_lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static inline void tft_wait_for_busy(struct tft_priv *priv)
{
    while(gpio_get(priv->gpio.blk));
}

static int tft_eink_luatos_init_display(struct tft_priv *priv)
{
    gpio_init(priv->gpio.blk);
    gpio_set_dir(priv->gpio.blk, GPIO_IN);

    printf("%s, writing patched initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    tft_wait_for_busy(priv);

    // driver output control
    // GD = 0; SM = 0; TB = 0;
    write_reg(priv, 0x01, (TFT_VER_RES - 1) & 0xFF, ((TFT_VER_RES - 1) >> 8) & 0xFF, 0x00);

    // booster soft start
    write_reg(priv, 0x0C, 0xD7, 0xD6, 0x9D);

    // write vcom register
    write_reg(priv, 0x2C, 0xA8); // VCOM 7C

    // set dummy line period
    write_reg(priv, 0x3A, 0x1A); // 4 dummy lines per gate

    // set gate line width
    write_reg(priv, 0x3B, 0x08); // 2us per line

    write_reg(priv, 0x11, 0x03);
    
    // set the loop-up table register
    write_reg(priv, 0x32, 0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
}

static void tft_eink_luatos_set_refresh_mode(struct tft_priv *priv, u8 mode)
{
    if (mode == EINK_LUATOS_REFRESH_PART) {
        write_reg(priv, 0x32, 0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    } else {
        write_reg(priv, 0x32, 0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
        0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
        0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
        0x35, 0x51, 0x51, 0x19, 0x01, 0x00);
    }
}

static void tft_eink_luatos_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe, int ye)
{
    /* set column adddress */
    write_reg(priv, 0x44, (xs >> 3) & 0xFF, (xe >> 3) & 0xFF);

    /* set row address */
    write_reg(priv, 0x45, ys & 0xFF, (ys >> 8) & 0xFF, ye & 0xFF, (ye >> 8) & 0xFF);
}

static void tft_eink_luatos_set_cursor(struct tft_priv *priv, int x, int y)
{
    write_reg(priv, 0x4E, (x >> 3) & 0xFF);
    write_reg(priv, 0x4F, y & 0xFF, (y >> 8) & 0xFF);
}

static void tft_eink_luatos_turn_on_dispaly(struct tft_priv *priv)
{
    write_reg(priv, 0x22, 0xC4);
    write_reg(priv, 0x20);
    write_reg(priv, 0xFF);
    
    tft_wait_for_busy(priv);
}

static void tft_eink_luatos_sleep(struct tft_priv *priv)
{
    write_reg(priv, 0x10, 0x01);
}

static int tft_eink_luatos_clear(struct tft_priv *priv, u16 clear)
{
    u8 width, height;
    width = ( TFT_HOR_RES % 8 == 0 ) ? ( TFT_HOR_RES / 8 ) :
            ( TFT_HOR_RES / 8 + 1 );
    height = TFT_VER_RES;

    tft_eink_luatos_set_addr_win(priv, 0, 0, TFT_HOR_RES, TFT_VER_RES);
    for( uint16_t i = 0; i < height; i++ ) {
        tft_eink_luatos_set_cursor(priv, 0, i );
        write_reg(priv, 0x24);

        for( uint16_t j = 0; j < width; j++ ) {
            write_buf_dc(priv, (u8[]){clear}, 1, 1);
        }
    }

    tft_eink_luatos_turn_on_dispaly(priv);
}

static uint8_t refresh_count = 0;
static void tft_eink_luatos_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    u8 *vmem8 = (uint8_t *)vmem;
    u8 width, height, data;
    width = ( TFT_HOR_RES % 8 == 0 ) ? ( TFT_HOR_RES / 8 ) :
            ( TFT_HOR_RES / 8 + 1 );
    height = TFT_VER_RES;

    tft_eink_luatos_set_addr_win(priv, 0, 0, TFT_HOR_RES, TFT_VER_RES);

    for (u8 i = 0; i < height; i++) {
        tft_eink_luatos_set_cursor(priv, 0, i);
        write_reg(priv,  0x24);

        for (u8 j = 0; j < width; j++) {
            data = vmem8[j + i * 25];
            write_buf_dc(priv, &data, 1, 1);
        }
    }
    refresh_count++;

    if (refresh_count % 8 == 0) {
        tft_eink_luatos_set_refresh_mode(priv, EINK_LUATOS_REFRESH_FULL);
        tft_eink_luatos_turn_on_dispaly(priv);
    } else {
        tft_eink_luatos_set_refresh_mode(priv, EINK_LUATOS_REFRESH_PART);
        tft_eink_luatos_turn_on_dispaly(priv);
    }
}

struct tft_display eink_luatos = {
    .xres = TFT_X_RES,
    .yres = TFT_Y_RES,
    .bpp  = 16,
    .backlight = 100,
    .need_tx_buf = true,
        .tftops = {
        .write_reg = tft_write_reg8,
        .init_display = tft_eink_luatos_init_display,
        .set_addr_win = tft_eink_luatos_set_addr_win,
        .video_sync = tft_eink_luatos_video_sync,
        .clear = tft_eink_luatos_clear,
    },
};

int tft_driver_init(void)
{
    tft_probe(&eink_luatos);
    return 0;
}

#endif
