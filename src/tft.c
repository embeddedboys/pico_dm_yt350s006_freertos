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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pico/time.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/platform.h"
#include "pico/stdio_uart.h"
#include "pico/binary_info.h"

#include "hardware/spi.h"
#include "hardware/pwm.h"

#include "tft.h"

#define TAG "tft: "

#define pr_debug(...) printf(TAG __VA_ARGS__)
#define pr_debug_nt(...)

static struct tft_priv g_priv;

static TaskHandle_t xTaskToNotify = NULL;
static const UBaseType_t XArrayIndex = 1;

void tft_spi_write_buf_dc(struct tft_priv *priv, void *buf, size_t len, bool dc)
{
    gpio_put(TFT_DC_PIN, dc);

    dm_gpio_set_value(TFT_CS_PIN, 0);
    spi_write_blocking(spi_ifce, buf, len);
    dm_gpio_set_value(TFT_CS_PIN, 1);
}

#define define_tft_write_reg(func, reg_type) \
void func(struct tft_priv *priv, int len, ...)  \
{   \
    reg_type *buf = (reg_type *)priv->buf; \
    va_list args;   \
    int i;  \
    \
    va_start(args, len);    \
    *buf = (reg_type)va_arg(args, unsigned int); \
    pr_debug_nt("cmd : 0x%02x\n", *buf); \
    write_buf_dc(priv, buf, sizeof(reg_type), 0); \
    len--;  \
    \
    /* if there no privams */  \
    if (len == 0)  \
        goto exit_no_param; \
    \
    pr_debug_nt(" val :"); \
    for (i = 0; i < len; i++) { \
        pr_debug_nt(" 0x%02x", *buf); \
        *buf++ = (reg_type)va_arg(args, unsigned int); \
    }   \
    pr_debug_nt("\n"); \
    \
    len *= sizeof(reg_type);    \
    write_buf_dc(priv, priv->buf, len, 1);  \
exit_no_param:  \
    va_end(args);   \
}

define_tft_write_reg(tft_write_reg8, u8)
define_tft_write_reg(tft_write_reg16, u16)

static int tft_reset(struct tft_priv *priv)
{
    dm_gpio_set_value(priv->gpio.reset, 1);
    mdelay(10);
    dm_gpio_set_value(priv->gpio.reset, 0);
    mdelay(10);
    dm_gpio_set_value(priv->gpio.reset, 1);
    mdelay(10);
    return 0;
}

static void inline tft_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe,
                                int ye)
{
    /* set column adddress */
    write_reg(priv, 0x2A, xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);

    /* set row address */
    write_reg(priv, 0x2B, ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);

    /* write start */
    write_reg(priv, 0x2C);
}

static int tft_clear(struct tft_priv *priv, u16 clear)
{
    u32 width = priv->display->xres;
    u32 height = priv->display->yres;
    int x, y;

    pr_debug("clearing screen (%d x %d) with color 0x%x\n", width, height, clear);

    priv->tftops->set_addr_win(priv, 0, 0,
                         priv->display->xres - 1,
                         priv->display->yres - 1);

    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            write_buf_dc(priv, &clear, sizeof(u16), 1);
        }
    }

    return 0;
}

static void tft_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    // pr_debug("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);
    write_buf_dc(priv, vmem, len, 1);
}

void tft_video_flush(int xs, int ys, int xe, int ye, void *vmem, uint32_t len)
{
    xTaskToNotify = xTaskGetCurrentTaskHandle();

    g_priv.tftops->video_sync(&g_priv, xs, ys, xe, ye, vmem, len);

    xTaskNotifyGiveIndexed(xTaskToNotify, XArrayIndex);

    xTaskToNotify = NULL;
}

portTASK_FUNCTION(video_flush_task, pvParameters)
{
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 100 );
    uint32_t ulNotificationValue;
    struct video_frame vf;

    for (;;) {
        /* if lvgl request to draw */
        if (xQueueReceive(xToFlushQueue, &vf, portMAX_DELAY)) {
            // pr_debug("Received video frame to flush\n");
            tft_video_flush(vf.xs, vf.ys, vf.xe, vf.ye, vf.vmem, vf.len);

            /* waiting for notification */
            ulNotificationValue = ulTaskNotifyTakeIndexed(XArrayIndex, pdTRUE, xMaxBlockTime);
            // pr_debug("Received notification, val : %d\n", ulNotificationValue);

            if (ulNotificationValue > 0) {
                call_lv_disp_flush_ready();
            } else {
                /* timeout */
            }
        }
    }

    vTaskDelete(NULL);
}

void tft_async_video_flush(struct video_frame *vf)
{
    xQueueSend(xToFlushQueue, (void *)vf, portMAX_DELAY);
}

static int tft_gpio_init(struct tft_priv *priv)
{
    pr_debug("initializing gpios...\n");

    gpio_init(priv->gpio.cs);
    gpio_set_dir(priv->gpio.cs, GPIO_OUT);
    gpio_put(priv->gpio.cs, 1);
    // bi_decl(bi_1pin_with_name(priv->gpio.cs, "SPI CS"));

    gpio_init(priv->gpio.reset);
    gpio_set_dir(priv->gpio.reset, GPIO_OUT);
    // bi_decl(bi_1pin_with_name(priv->gpio.reset, "TFT RES"));

    gpio_init(priv->gpio.dc);
    gpio_set_dir(priv->gpio.dc, GPIO_OUT);
    // bi_decl(bi_1pin_with_name(priv->gpio.dc, "TFT DC"));

    gpio_init(priv->gpio.blk);
    gpio_set_dir(priv->gpio.blk, GPIO_OUT);

    // gpio_set_function(priv->gpio.blk, GPIO_FUNC_PWM);
    // bi_decl(bi_1pin_with_name(priv->gpio.blk, "TFT BLK"));

    // uint32_t slice_num = pwm_gpio_to_slice_num(priv->gpio.blk);
    // pwm_config config = pwm_get_default_config();
    // pwm_config_set_clkdiv(&config, 4.f);
    // pwm_init(slice_num, &config, true);
}

static int tft_hw_init(struct tft_priv *priv)
{
    int ret;

    pr_debug("%s\n", __func__);

    pr_debug("TFT interface type: %s\n", DISP_OVER_PIO ? "PIO" : "SPI");

#if DISP_OVER_PIO
    pio_spi_tx_init(TFT_SDA_PIN, TFT_SCL_PIN);
    bi_decl(bi_2pins_with_func(TFT_SCL_PIN, TFT_SDA_PIN, GPIO_FUNC_PIO0));
#else
    spi_init(spi_ifce, TFT_BUS_CLK_KHZ * 1000);
    gpio_set_function(TFT_SCL_PIN, GPIO_FUNC_SPI);
    gpio_set_function(TFT_SDA_PIN, GPIO_FUNC_SPI);
    bi_decl(bi_2pins_with_func(TFT_SCL_PIN, TFT_SDA_PIN, GPIO_FUNC_SPI));
    pr_debug("spi%d initialized at %d kHz\n", spi_get_index(spi_ifce), spi_get_baudrate(spi_ifce) / 1000 );
#endif

    tft_gpio_init(priv);

    if (!priv->tftops->init_display) {
        // pr_error("init_display must be provided\n");
        return -1;
    }

    pr_debug("initializing display...\n");
    priv->tftops->init_display(priv);

    /* clear screen to black */
    pr_debug("clearing screen...\n");
    priv->tftops->clear(priv, 0x0);

    pr_debug("enbaling backlight...\n");
    dm_gpio_set_value(priv->gpio.blk, 1);
}

// static void tft_set_backlight(u16 level)
// {
//     pwm_set_gpio_level(TFT_BLK_PIN, level * level);
// }

void tft_merge_tftops(struct tft_ops *dst, struct tft_ops *src)
{
    pr_debug("%s\n", __func__);
    if (src->write_reg)
        dst->write_reg = src->write_reg;
    if (src->init_display)
        dst->init_display = src->init_display;
    if (src->reset)
        dst->reset = src->reset;
    if (src->clear)
        dst->clear = src->clear;
    if (src->sleep)
        dst->sleep = src->sleep;
    if (src->set_addr_win)
        dst->set_addr_win = src->set_addr_win;
    if (src->video_sync)
        dst->video_sync = src->video_sync;
}

int tft_probe(struct tft_display *display)
{
    struct tft_priv *priv = &g_priv;
    pr_debug("%s\n", __func__);

    priv->buf = (u8 *)malloc(TFT_REG_BUF_SIZE);
    if (!priv->buf) {
        pr_debug("failed to allocate buffer\n");
        return -1;
    }

    priv->tftops = (struct tft_ops *)malloc(sizeof(struct tft_ops));
    if (!priv->tftops) {
        pr_debug("failed to allocate tftops\n");
        goto exit_free_priv_buf;
    }

    priv->display = display;

    priv->gpio.scl   = TFT_SCL_PIN;
    priv->gpio.sda   = TFT_SDA_PIN;
    priv->gpio.reset = TFT_RES_PIN;
    priv->gpio.dc    = TFT_DC_PIN;
    priv->gpio.cs    = TFT_CS_PIN;
    priv->gpio.blk   = TFT_BLK_PIN;

    priv->tftops->reset = tft_reset;
    priv->tftops->set_addr_win = tft_set_addr_win;
    priv->tftops->clear = tft_clear;
    priv->tftops->video_sync = tft_video_sync;

    if (priv->display->need_tx_buf) {
        priv->txbuf.buf = (u8 *)malloc(TFT_TX_BUF_SIZE);
        if (!priv->txbuf.buf) {
            pr_debug("failed to allocate tx buffer\n");
            goto exit_free_tftops;
        }
        priv->txbuf.len = TFT_TX_BUF_SIZE;
    }

    tft_merge_tftops(priv->tftops, &display->tftops);

    tft_hw_init(priv);

    return 0;

exit_free_tftops:
    free(priv->tftops);
exit_free_priv_buf:
    free(priv->buf);
    return -1;
}
