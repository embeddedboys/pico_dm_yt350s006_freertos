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

#if LCD_DRV_USE_ILI9481

static int tft_ili9481_init_display(struct tft_priv *priv)
{
    printf("%s, writing patched initial sequence...\n", __func__);
    priv->tftops->reset(priv);

    write_reg(priv, 0x11);
    mdelay(20);

    write_reg(priv, 0xD0, 0x07, 0x42, 0x18);

    write_reg(priv, 0xD1, 0x00, 0x07, 0x10);

    write_reg(priv, 0xD2, 0x01, 0x02);

    write_reg(priv, 0xC0, 0x10, 0x3B, 0x00, 0x02, 0x11);

    write_reg(priv, 0xC5, 0x03);

    write_reg(priv, 0xC8, 0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00);

    write_reg(priv, 0x36, 0x28);

    write_reg(priv, 0x3A, 0x66);

    write_reg(priv, 0x11);
    // write_reg(priv, 0x21);
    mdelay(120);
    write_reg(priv, 0x29);

    return 0;
}

#define TX_BUF_SIZE (9216)
#define TX_ARRAY_SIZE (TX_BUF_SIZE / 3)
u8 tx_buf[TX_BUF_SIZE];
static void tft_ili9481_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    u16 *vmem16 = (u16 *)vmem;
    size_t remain = len / 2;
    static size_t to_send;
    u16 color;

    // pr_debug("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);

    while (remain) {
        to_send = MIN(remain, TX_ARRAY_SIZE);

        for (int i = 0, j = 0; j < MIN(remain, TX_ARRAY_SIZE); i+=3, j++) {
            color = vmem16[j] << 8 | vmem16[j] >> 8;
            tx_buf[i]   = (color >> 8) & 0xF8;
            tx_buf[i+1] = (color >> 3) & 0xFC;
            tx_buf[i+2] = (color << 3);
        }

        write_buf_dc(priv, tx_buf, to_send * 3, 1);

        remain -= to_send;
        vmem16 += to_send;
    }

}

static struct tft_display ili9481 = {
    .xres = TFT_X_RES,
    .yres = TFT_Y_RES,
    .bpp  = 16,
    .backlight = 100,
    .tftops = {
        .write_reg = tft_write_reg8,
        .init_display = tft_ili9481_init_display,
        .video_sync = tft_ili9481_video_sync,
    },
};

int tft_driver_init(void)
{
    tft_probe(&ili9481);
    return 0;
}

#endif
