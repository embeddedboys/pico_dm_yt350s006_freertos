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

#if LCD_DRV_USE_ST7735

static int tft_st7735_init_display(struct tft_priv *priv)
{
    printf("%s, writing patched initial sequence...\n", __func__);
    priv->tftops->reset(priv);

    write_reg(priv, 0x11);
    mdelay(120);

    write_reg(priv, 0x36, (1 << 7) | (1 << 6) | (1 << 5));
    write_reg(priv, 0x3A, 0x55);

    write_reg(priv, 0x21);
    write_reg(priv, 0x29);
}

static void inline tft_st7735_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe, int ye)
{
    // Apply offset from display spec.
    xs += priv->display->xoffs;
    xe += priv->display->xoffs;
    ys += priv->display->yoffs;
    ye += priv->display->yoffs;

    /* set column adddress */
    write_reg(priv, 0x2A, xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);

    /* set row address */
    write_reg(priv, 0x2B, ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);

    /* write start */
    write_reg(priv, 0x2C);
}

static struct tft_display st7735 = {
    .xres = TFT_X_RES,
    .yres = TFT_Y_RES,
    .xoffs = 0,
    .yoffs = 25,
    .bpp  = 16,
    .backlight = 100,
    .tftops = {
        .write_reg = tft_write_reg8,
        .init_display = tft_st7735_init_display,
        .set_addr_win = tft_st7735_set_addr_win,
    },
};

int tft_driver_init(void)
{
    tft_probe(&st7735);
    return 0;
}

#endif
