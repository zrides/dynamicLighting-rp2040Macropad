#ifndef OLED_H
#define OLED_H

void sh1106_init();

void sh1106_write_command(uint8_t command);
void sh1106_write_data(uint8_t *data, size_t size);
void sh1106_clear_display();

void sh1106_render_image(uint8_t *image, uint8_t width, uint8_t height, uint8_t start_column, uint8_t start_page);

void sh1106_update_display();

#endif