#include <stdint.h>
#include "include/vga.h"
volatile uint16_t* vga_buff = (uint16_t*)0xb8000; //global pointer to the video memory
uint16_t vga_index = 0; //global variable to track where the cursor is currently sitting on the screen
static const uint8_t default_color = 0x0F; //White text (F) on Black background (0)

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

void vga_print(const char* str) {
    while (*str != '\0') {
        if (*str == '\n') { //if the character is a newline (\n). If it is, calculate the next row
            vga_index = (vga_index / 80 + 1) * 80; //VGA screen is exactly 80 characters wide
            /*
             *vga_index / 80: Integer division drops the remainder entirely. 85 / 80 = 1.
             *This tells us that you are on Row 1.+ 1: Moves our target to the next physical line. 1 + 1 = 2.
             *we want Row 2.* 80: Multiplies by the screen width to find the starting index number of that new row. 2 * 80 = 160. which is the absolute beginning of the next line!
             */
        }else {
            uint16_t colored_char=(default_color<<8)|*str;
            /*
             *16-bit pointer (uint16_t) writes 2 bytes at the same time
             * default_color << 8: Take our color byte (like white-on-black: 0x0F, which is binary 00001111) and slide it 8 spots to the left.
             * It becomes: [ 00001111 | 00000000 ]
             *
             * | (uint8_t)(*str): Drop our character byte (like 'A', which is binary 01000001) into the empty right side using a logical Bitwise OR (|).
             * final joined result is: [ 00001111 | 01000001 ]
             */
            vga_buff[vga_index] = colored_char;
            vga_index++;
        }
        str++; // Move to the next text character pointer in RAM

    }
}