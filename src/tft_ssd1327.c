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

#if LCD_DRV_USE_SSD1327

static int tft_ssd1327_init_display(struct tft_priv *priv)
{
    printf("%s, writing patched initial sequence...\n", __func__);
    priv->tftops->reset(priv);

    write_reg(priv,  0xae);

    write_reg(priv,  0x15);
    write_reg(priv,  0x00);
    write_reg(priv,  0x7f);

    write_reg(priv,  0x75);
    write_reg(priv,  0x00);
    write_reg(priv,  0x7f);

    write_reg(priv,  0x81);
    write_reg(priv,  0x80);

    write_reg(priv,  0xa0);
    write_reg(priv,  0x51);

    write_reg(priv,  0xa1);
    write_reg(priv,  0x00);

    write_reg(priv,  0xa2);
    write_reg(priv,  0x00);

    write_reg(priv,  0xa4);
    write_reg(priv,  0xa8);
    write_reg(priv,  0x7f);

    write_reg(priv,  0xb1);
    write_reg(priv,  0xf1);

    write_reg(priv,  0xb3);
    write_reg(priv,  0x00);

    write_reg(priv,  0xab);
    write_reg(priv,  0x01);

    write_reg(priv,  0xb6);
    write_reg(priv,  0x0f);

    write_reg(priv,  0xbe);
    write_reg(priv,  0x0f);

    write_reg(priv,  0xbc);
    write_reg(priv,  0x08);

    write_reg(priv,  0xd5);
    write_reg(priv,  0x62);

    write_reg(priv,  0xfd);
    write_reg(priv,  0x12);

    mdelay(200);
    write_reg(priv,  0xaf);

    return 0;
}

static void tft_ssd1327_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe, int ye)
{
    write_reg(priv, 0x15);
    write_reg(priv, (xs / 2));
    write_reg(priv, (xe / 2));

    write_reg(priv, 0x75);
    write_reg(priv, ys);
    write_reg(priv, ye);
}

#define RED(a)      ((((a) & 0xf800) >> 11) << 3)
#define GREEN(a)    ((((a) & 0x07e0) >> 5) << 2)
#define BLUE(a)     (((a) & 0x001f) << 3)
#define to_rgb565(r,g,b) ((r) << 11 | (g) << 5 | (b))
static inline u16 rgb565_to_grayscale_by_average(u16 rgb565)
{
        int r, g, b;
        u16 gray;

        r = RED(rgb565);
        g = GREEN(rgb565);
        b = BLUE(rgb565);

        gray = ((r + g + b) / 3);

        /* map to rgb565 format */
        r = b = gray * 31 / 255;  // 0 ~ 31
        g = gray * 63 / 255;

        return to_rgb565(r, g, b);
}

static inline u8 rgb565_to_4bit_grayscale(u16 rgb565)
{
        int r, g, b;
        int level;
        u16 gray;

        /* get each channel and expand them to 8 bit */
        r = RED(rgb565);
        g = GREEN(rgb565);
        b = BLUE(rgb565);

        /* convert rgb888 to grayscale */
        gray = ((r * 77 + g * 151 + b * 28) >> 8); // 0 ~ 255
        if (gray == 0)
                return gray;

        /*
         * so 4-bit grayscale like:
         * B3  B2  B1  B0
         * 0   0   0   0
         * which means have 16 kind of gray
         */
        gray /= 16;

        return gray;
}

static void tft_ssd1327_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    // pr_debug("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    size_t remain, to_copy, tx_array_size;
    u8 *tx_buf = (u8 *)priv->txbuf.buf;
    u16 *vmem16 = (u16 *)vmem;
    u8 p0, p1;
    int i, j;

    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);

    remain = len / 2;
    tx_array_size = priv->txbuf.len;

    while (remain) {
        to_copy = MIN(tx_array_size, remain);

        for (i = 0, j = 0; i < to_copy; i += 2, j++) {
            p0 = rgb565_to_4bit_grayscale(vmem16[i]);
            p1 = rgb565_to_4bit_grayscale(vmem16[i+1]);
            tx_buf[j] = (p0 << 4) | p1;
        }

        vmem16 = vmem16 + to_copy;

        write_buf_dc(priv, tx_buf, to_copy / 2, 1);

        remain -= to_copy;
    }

}

static struct tft_display ssd1327 = {
    .xres = TFT_X_RES,
    .yres = TFT_Y_RES,
    .bpp  = 16,
    .backlight = 100,
    .need_tx_buf = true,
    .tftops = {
        .write_reg = tft_write_reg8,
        .init_display = tft_ssd1327_init_display,
        .set_addr_win = tft_ssd1327_set_addr_win,
        .video_sync = tft_ssd1327_video_sync,
    },
};

int tft_driver_init(void)
{
    tft_probe(&ssd1327);
    return 0;
}

#endif