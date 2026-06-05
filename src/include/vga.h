#ifndef VGA_H
#define VGA_H
#include <stdint.h>
void vga_print(const char* str);
void vga_hex_print(uint32_t num);
void vga_print_hex_64(uint64_t num); //64-bit handler
void vga_print_dec(uint32_t num);
void vga_clear_screen();
void vga_print_color(const char* str, uint8_t color);
void klog_ok(const char* message);
void klog_err(const char* message);
void os_greeting(const char* message);
#endif