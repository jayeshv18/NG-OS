#include <stdint.h>
#include "include/vga.h"
volatile uint16_t* vga_buff = (uint16_t*)0xb8000; //global pointer to the video memory
uint16_t vga_index = 0; //global variable to track where the cursor is currently sitting on the screen
static const uint8_t default_color = 0x0F; //White text (F) on Black background (0)
static const uint8_t green_color = 0x0A;
static const uint8_t red_color = 0x0C;
static const uint8_t cyan_color = 0x03;
/*
 *Clears the entire 80x25 text screen by writing directly to the video card's memory.
*Because 'vga_buff' is a pointer aimed at the physical VGA address (0xB8000),
*assigning values to 'vga_buff[i]' bypasses regular RAM and sends data straight
*to the graphics hardware, filling all 2000 cells with blank white-on-black spaces.
*standard VGA screen is exactly 80 columns wide and 25 rows tall. 80 x 25 = 2000
*/


void vga_clear_screen() {
    for (int i=0;i<=1999;i++) {
        uint16_t emp_char=(default_color<<8)|' ';
        vga_buff[i]=emp_char; //sending those characters to vga_buff[i] because vga_buff is a direct pointer to the video memory address 0xB8000.
        //Take the empty character data (emp_char) and drop it directly into the physical screen slot ith inside the video card
    }
    vga_index=0; //reset our global vga_index back to 0 so the next time vga_print is called, it starts at the absolute top-left corner.
}

/* the code has been updated to introduce logging prefixes.
void vga_print(const char* str) {
    while (*str != '\0') {
        if (*str == '\n') { //if the character is a newline (\n). If it is, calculate the next row
            vga_index = (vga_index / 80 + 1) * 80; //VGA screen is exactly 80 characters wide

             //vga_index / 80: Integer division drops the remainder entirely. 85 / 80 = 1.
             //This tells us that we are on Row 1.+ 1: Moves our target to the next physical line. 1 + 1 = 2.
             //we want Row 2.* 80: Multiplies by the screen width to find the starting index number of that new row. 2 * 80 = 160. which is the absolute beginning of the next line!

        }else {
            uint16_t colored_char=(default_color<<8)|*str;

             //16-bit pointer (uint16_t) writes 2 bytes at the same time
             // default_color << 8: Take our color byte (like white-on-black: 0x0F, which is binary 00001111) and slide it 8 spots to the left.
             // It becomes: [ 00001111 | 00000000 ]

             // | (uint8_t)(*str): Drop our character byte (like 'A', which is binary 01000001) into the empty right side using a logical Bitwise OR (|).
             // final joined result is: [ 00001111 | 01000001 ]

            vga_buff[vga_index] = colored_char;
            vga_index++;
        }
        str++; // Move to the next text character pointer in RAM
        //scroll logic
        //if the cursor has fallen off the bottom of the screen (25 rows * 80 cols = 2000)
        if (vga_index >= 2000) {
            //shift the first 24 rows up by exactly one row (80 slots forward)
            for (int i = 0; i < 24 * 80; i++) {
                vga_buff[i] = vga_buff[i + 80];
            }
            //wipe the 25th row clean with blank spaces
            for (int i = 24 * 80; i < 2000; i++) {
                vga_buff[i] = (default_color << 8) | ' ';
            }
            //snap the cursor back to the beginning of the bottom row
            vga_index = 24 * 80;
        }
    }
}
*/


void vga_hex_print(uint32_t num) {
    //32-bit hex number has exactly 8 digits.
    char hex_str[11]="0x00000000"; //The 11 is for 0x, the 8 digits, and the \0 null terminator
    char* hex_digits = "0123456789ABCDEF"; //array lookup
    for (int i = 9; i >= 2; i--) {//we start at index 9 (the very last digit slot before '\0') and move backwards. Hex is counted from the back.
        uint8_t nibble=num & 0x0F;
        /*
         * number 0x0F in binary is written as: 0000 1111.
         * sample 8-bit number like 0x4A. In binary, 0x4A is written as: 0100 1010.
        *     0100 1010  (The original number: 0x4A)
            & 0000 1111  (The mask/stencil:    0x0F)
              ───────────
              0000 1010  (result is 10 ie A)
         */
        hex_str[i]=hex_digits[nibble];
        num >>= 4; // Before shift: 0100 1010 (0x4A), After shift:  0000 0100 because the other half is also need to be decoded.
    }
    vga_print(hex_str); //hand the finished string to our existing VGA print driver
}
void vga_print_hex_64(uint64_t num) { //this function uses bitwise operations to slice the uint64_t into two uint32_t pieces (a "high" half and a "low" half).
    //this provides a precise 32bit without lingering nums.
    uint32_t high=(uint32_t)(num>>32); //the first 32 bits are shifted to the lower side and the lower side is complete removed, so that the higher 32 gets space to shift.
    uint32_t low=(uint32_t)(num&0xFFFFFFFF); //0xFFFFFFFF means a solid wall of thirty-two 0s on the left, and thirty-two 1s on the right.
        //bitwise AND helps to completely remove the high 32 and only keep the lower 32.

    //printing the high half, then immediately print the low half, because we don't print a newline (\n) between them, they will appear on the screen as one massive, continuous 64-bit hex number!
    vga_print("");//prefix
    vga_hex_print(high);
    vga_hex_print(low);
}

void vga_print_dec(uint32_t num) {
    if (num==0) {
        vga_print("0");
        return;
    }
    char buffer[11]; //max 32-bit integer is 10 digits + null terminator
    int i=0;

    //extract digits in reverse order
    while (num>0) {
        buffer[i]=(num%10)+'0'; //convert math number to ASCII character
        num=num/10;
        i++;
    }

    while (i>0) {
        i--;
        //way to print one char at a time since we only have string printing
        char single_char[2]={buffer[i],'\0'};
        vga_print(single_char);
    }
}

void vga_print_color(const char* str, uint8_t color) {
    while (*str != '\0') {
        if (*str == '\n') {
            vga_index = (vga_index / 80 + 1) * 80;
        }else {
            uint16_t colored_char=(color<<8)|*str;
            vga_buff[vga_index] = colored_char;
            vga_index++;
        }
        str++;
        if (vga_index >= 2000) {
            //shift the first 24 rows up by exactly one row (80 slots forward)
            for (int i = 0; i < 24 * 80; i++) {
                vga_buff[i] = vga_buff[i + 80];
            }
            //wipe the 25th row clean with blank spaces
            for (int i = 24 * 80; i < 2000; i++) {
                vga_buff[i] = (color << 8) | ' ';
            }
            //snap the cursor back to the beginning of the bottom row
            vga_index = 24 * 80;
        }
    }
}
void vga_print(const char* str) { //wrapper function
    vga_print_color(str, default_color);
}

void klog_ok(const char* message) {
    vga_print_color("[OK] ", green_color); // Prints prefix in Green
    vga_print_color(message, default_color); // Prints message in White
    vga_print_color("\n", default_color);    // Newline
}

void klog_err(const char* message) {
    vga_print_color("[!] ", red_color);    // Prints prefix in Red
    vga_print_color(message, default_color); // Prints message in White
    vga_print_color("\n", default_color);    // Newline
}

void os_greeting(const char* message) {
    vga_print_color(message, cyan_color); // Prints message in White
    vga_print_color("\n", default_color);    // Newline
}