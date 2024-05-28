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

#include "indev.h"
#include "debug.h"

#if INDEV_DRV_USE_CST816D

#define CST816D_ADDR 0x12
#define CST816D_DEF_SPEED 400000
#define CST816D_PIN_SCL 27
#define CST816D_PIN_SDA 26
#define CST816D_PIN_IRQ 21
#define CST816D_PIN_RST 29

extern int i2c_bus_scan(i2c_inst_t *i2c);

static void cst816d_write_reg(struct indev_priv *priv, u8 reg, u8 val)
{

}

static u8 cst816d_read_reg(struct indev_priv *priv, u8 reg)
{
    return 0;
}

static void cst816d_hw_init(struct indev_priv *priv)
{
    pr_debug("%s\n", __func__);

    pr_debug("initialzing i2c controller\n");
    i2c_init(priv->spec->i2c.master, priv->spec->i2c.speed);

    pr_debug("set gpio i2c function\n");
    gpio_init(priv->spec->i2c.pin_scl);
    gpio_init(priv->spec->i2c.pin_sda);
    gpio_set_function(priv->spec->i2c.pin_scl, GPIO_FUNC_I2C);
    gpio_set_function(priv->spec->i2c.pin_sda, GPIO_FUNC_I2C);

    pr_debug("pull up i2c gpio\n");
    gpio_pull_up(priv->spec->i2c.pin_scl);
    gpio_pull_up(priv->spec->i2c.pin_sda);

    pr_debug("initialzing reset pin\n");
    gpio_init(priv->spec->pin_irq);
    gpio_set_dir(priv->spec->pin_irq, GPIO_IN);
    gpio_pull_up(priv->spec->pin_irq);

    pr_debug("initialzing reset pin\n");
    gpio_init(priv->spec->pin_rst);
    gpio_set_dir(priv->spec->pin_rst, GPIO_OUT);
    gpio_pull_up(priv->spec->pin_rst);

    pr_debug("chip reset\n");
    priv->ops->reset(priv);
    priv->ops->set_dir(priv, INDEV_DIR_SWITCH_XY | INDEV_DIR_INVERT_Y);

    i2c_bus_scan(priv->spec->i2c.master);
}

static bool cst816d_is_pressed(struct indev_priv *priv)
{

}

static struct indev_spec cst816d = {
    .name = "cst816d",
    .type = INDEV_TYPE_POINTER,

    .i2c = {
        .addr    = CST816D_ADDR,
        .master  = i2c1,
        .speed   = CST816D_DEF_SPEED,
        .pin_scl = CST816D_PIN_SCL,
        .pin_sda = CST816D_PIN_SDA,
    },

    .x_res = TOUCH_X_RES,
    .y_res = TOUCH_Y_RES,
    .x_offs = 0,
    .y_offs = 0,

    .pin_irq = CST816D_PIN_IRQ,
    .pin_rst = CST816D_PIN_RST,

    .ops = {
        .write_reg = cst816d_write_reg,
        .read_reg = cst816d_read_reg,
        .init = cst816d_hw_init,
        .is_pressed = cst816d_is_pressed,
    },
};

int indev_driver_init(void)
{
    printf("%s\n", __func__);
    indev_probe(&cst816d);
    return 0;
}

#endif
