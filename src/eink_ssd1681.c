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

#if LCD_DRV_USE_SSD1681

static inline void tft_wait_for_busy(struct tft_priv *priv)
{
    while(gpio_get(priv->gpio.blk));
}

static int tft_ssd1681_init_display(struct tft_priv *priv)
{
    gpio_init(priv->gpio.blk);
    gpio_set_dir(priv->gpio.blk, GPIO_IN);

    printf("%s, writing patched initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    tft_wait_for_busy(priv);

    write_reg(priv, 0x12);
    tft_wait_for_busy(priv);

    write_reg(priv, 0x01, 0xC7, 0x00, 0x01);

    write_reg(priv, 0x11, 0x01);

    write_reg(priv, 0x44, 0x00, 0x18);

    write_reg(priv, 0x45, 0xC7, 0x00, 0x00, 0x00);

    write_reg(priv, 0x3C, 0x05);

    write_reg(priv, 0x18, 0x80);

    write_reg(priv, 0x4E, 0x00, 0x4F, 0xC7, 0x00);
    tft_wait_for_busy(priv);
    
    priv->tftops->clear(priv, 0xFFFF);
}

static void tft_ssd1681_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe, int ye)
{
    /* set column adddress */
    write_reg(priv, 0x44, xs & 0xFF, xe & 0xFF);

    /* set row address */
    write_reg(priv, 0x45, ys & 0xFF, (ys >> 8) & 0xFF, ye & 0xFF, (ye >> 8) & 0xFF);
}

static void tft_ssd1681_set_cursor(struct tft_priv *priv, int x, int y)
{
    write_reg(priv, 0x4E, x & 0xFF);
    write_reg(priv, 0x4F, y & 0xFF, (y >> 8) & 0xFF);
}

static void tft_part_refresh(struct tft_priv *priv)
{
    write_reg(priv, 0x22, 0xFF);
    write_reg(priv, 0x20);
    tft_wait_for_busy(priv);
}

static void tft_full_refresh(struct tft_priv *priv)
{
    write_reg(priv, 0x22, 0xF7);
    write_reg(priv, 0x20);
    tft_wait_for_busy(priv);
}

static void tft_fast_refresh(struct tft_priv *priv)
{
    write_reg(priv, 0x22, 0xC7);
    write_reg(priv, 0x20);
    tft_wait_for_busy(priv);
}

static int tft_ssd1681_clear(struct tft_priv *priv, u16 clear)
{
    uint8_t width, height;
    width = ( TFT_HOR_RES % 8 == 0 ) ? ( TFT_HOR_RES / 8 ) :
            ( TFT_HOR_RES / 8 + 1 );
    height = TFT_VER_RES;
    int i, j;

    priv->tftops->reset(priv);
    tft_wait_for_busy(priv);
    write_reg(priv, 0x3C, 0x80);

    write_reg(priv, 0x44, 0x00, 0x18);
    write_reg(priv, 0x45, 0xC7, 0x00, 0x00, 0x00);

    write_reg(priv, 0x24);
    for (i = 0; i < TFT_HOR_RES * height; i++) {
        write_buf_dc(priv, (u8[]){0xFF}, 1, 1);
    }

    tft_full_refresh(priv);
}

static uint8_t refresh_count = 0;
static void tft_ssd1681_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    uint8_t *vmem8 = (uint8_t *)vmem;

    priv->tftops->reset(priv);
    tft_wait_for_busy(priv);

    len /= 16;

    write_reg(priv, 0x3C, 0x80);

    write_reg(priv, 0x44, 0x00, 0x18);
    write_reg(priv, 0x45, 0xC7, 0x00, 0x00, 0x00);
    write_reg(priv, 0x24);
    write_buf_dc(priv, vmem8, len, 1);
    refresh_count++;

    /* should do a full refresh after 8 times part refresh */
    if (refresh_count % 8 == 0)
        tft_full_refresh(priv);
    else
        tft_part_refresh(priv);
}

struct tft_display ssd1681 = {
    .xres = TFT_X_RES,
    .yres = TFT_Y_RES,
    .bpp  = 16,
    .backlight = 100,
    .need_tx_buf = true,
        .tftops = {
        .write_reg = tft_write_reg8,
        .init_display = tft_ssd1681_init_display,
        .set_addr_win = tft_ssd1681_set_addr_win,
        .video_sync = tft_ssd1681_video_sync,
        .clear = tft_ssd1681_clear,
    },
};

int tft_driver_init(void)
{
    tft_probe(&ssd1681);
    return 0;
}

#endif
