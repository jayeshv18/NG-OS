#ifndef VGA_H
#define VGA_H
#include <stdint.h>
void vga_print(const char* str);
void vga_hex_print(uint32_t num);
void vga_clear_screen();
#endif