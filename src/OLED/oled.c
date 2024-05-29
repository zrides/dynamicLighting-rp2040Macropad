/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"

#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "../util.h"
#include "pi_img.h"

#define SH1106_HEIGHT              64
#define SH1106_WIDTH               138

#define SH1106_SPI_PORT spi1


// SH1106 commands
#define SH1106_SET_CONTRAST 0x81
#define SH1106_DISPLAY_ALL_ON_RESUME 0xA4
#define SH1106_DISPLAY_ALL_ON 0xA5
#define SH1106_NORMAL_DISPLAY 0xA6
#define SH1106_INVERT_DISPLAY 0xA7
#define SH1106_DISPLAY_OFF 0xAE
#define SH1106_DISPLAY_ON 0xAF
#define SH1106_SET_DISPLAY_OFFSET 0xD3
#define SH1106_SET_COMPINS 0xDA
#define SH1106_SET_VCOM_DETECT 0xDB
#define SH1106_SET_DISPLAY_CLOCK_DIV 0xD5
#define SH1106_SET_PRECHARGE 0xD9
#define SH1106_SET_MULTIPLEX 0xA8
#define SH1106_SET_LOW_COLUMN 0x00
#define SH1106_SET_HIGH_COLUMN 0x10
#define SH1106_SET_START_LINE 0x40
#define SH1106_MEMORY_MODE 0x20
#define SH1106_COLUMN_ADDR 0x21
#define SH1106_PAGE_ADDR 0x22
#define SH1106_COM_SCAN_INC 0xC0
#define SH1106_COM_SCAN_DEC 0xC8
#define SH1106_SEG_REMAP 0xA0
#define SH1106_CHARGE_PUMP 0x8D

const uint8_t img_width = IMG_WIDTH;
const uint8_t img_height = IMG_HEIGHT;
static uint8_t x = (SH1106_WIDTH - img_width) / 2;
static uint8_t y = (SH1106_HEIGHT / 8 - img_height / 8) / 2;
static int8_t x_dir = 1;
static int8_t y_dir = 1;

static inline void gpio_init_and_set(uint gpio, bool value) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, value);
}

void sh1106_init() {
    // Initialize SPI
    spi_init(SH1106_SPI_PORT, 10000000); // 10 MHz
    gpio_set_function(OLED_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(OLED_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(OLED_MISO, GPIO_FUNC_SPI);

    // Initialize control pins
    gpio_init_and_set(OLED_CS, 1);
    gpio_init_and_set(OLED_DC, 0);
    gpio_init_and_set(OLED_RESET, 1);

    // Reset the display
    gpio_put(OLED_RESET, 0);
    sleep_ms(100);
    gpio_put(OLED_RESET, 1);

    // Initialization sequence
    uint8_t init_seq[] = {
        SH1106_DISPLAY_OFF,
        SH1106_SET_DISPLAY_CLOCK_DIV, 0x80,
        SH1106_SET_MULTIPLEX, 0x3F,
        SH1106_SET_DISPLAY_OFFSET, 0x00,
        SH1106_SET_START_LINE | 0x00,
        SH1106_CHARGE_PUMP, 0x14,
        SH1106_MEMORY_MODE, 0x00,
        SH1106_SEG_REMAP | 0x01,
        SH1106_COM_SCAN_DEC,
        SH1106_SET_COMPINS, 0x12,
        SH1106_SET_CONTRAST, 0xCF,
        SH1106_SET_PRECHARGE, 0xF1,
        SH1106_SET_VCOM_DETECT, 0x40,
        SH1106_DISPLAY_ALL_ON_RESUME,
        SH1106_NORMAL_DISPLAY,
        SH1106_DISPLAY_ON
    };

    gpio_put(OLED_CS, 0);
    gpio_put(OLED_DC, 0); // Command mode
    spi_write_blocking(SH1106_SPI_PORT, init_seq, sizeof(init_seq));
    gpio_put(OLED_CS, 1);
}

void sh1106_write_command(uint8_t command) {
    gpio_put(OLED_CS, 0);
    gpio_put(OLED_DC, 0); // Command mode
    spi_write_blocking(SH1106_SPI_PORT, &command, 1);
    gpio_put(OLED_CS, 1);
}

void sh1106_write_data(uint8_t *data, size_t size) {
    gpio_put(OLED_CS, 0);
    gpio_put(OLED_DC, 1); // Data mode
    spi_write_blocking(SH1106_SPI_PORT, data, size);
    gpio_put(OLED_CS, 1);
}

void sh1106_clear_display() {
    for (uint8_t page = 0; page < (SH1106_HEIGHT / 8); page++) {
        sh1106_write_command(0xB0 + page);  // Set page address
        sh1106_write_command(SH1106_SET_LOW_COLUMN);  // Set lower column start address
        sh1106_write_command(SH1106_SET_HIGH_COLUMN);  // Set higher column start address

        uint8_t clear_data[SH1106_WIDTH] = {0};
        sh1106_write_data(clear_data, SH1106_WIDTH);
    }
}

void sh1106_render_image(uint8_t *image, uint8_t width, uint8_t height, uint8_t start_column, uint8_t start_page) {
    for (uint8_t page = 0; page < (height / 8); page++) {
        sh1106_write_command(0xB0 + start_page + page);  // Set page address
        sh1106_write_command(start_column & 0x0F);  // Set lower column start address
        sh1106_write_command(0x10 | (start_column >> 4));  // Set higher column start address

        sh1106_write_data(&image[page * width], width);
    }
}

void sh1106_update_display() {
    sh1106_clear_display();
    sh1106_render_image(raspberry26x32, img_width, img_height, x, y);

    x += x_dir;
    y += y_dir;

    if (x <= 0 || x >= (SH1106_WIDTH - img_width)) {
        x_dir = -x_dir;
    }
    if (y <= 0 || y >= (SH1106_HEIGHT / 8 - img_height / 8)) {
        y_dir = -y_dir;
    }
}