 HEAD


#include <stdint.h>

//read a byte from a hardware port
/*
 * Reads a single byte (8 bits) from a specified hardware I/O port.
 *
 * static:   Limits the function scope to this file to prevent linking errors.
 * inline:   Requests the compiler embed the code directly to remove call overhead.
 * uint8_t:  The return type, representing a single 8-bit unsigned byte.
 * uint16_t: The port parameter type, matching the 16-bit x86 I/O space.
 */
static inline uint8_t inb(uint16_t port) {
    // variable to store the byte read from the hardware port
    uint8_t ret;
    /*
     * '=' means write only variable.
     * a ; forces the complier to use AL register
     * (ret), binds the value of AL to the C variable ie 'ret'.
     */

    /*
     * N, optimizes to a constant if the port number is 0-255.
     * d, forces the compiler to use DX regs if port > 255
     * (port), passes the C variable 'port' into the chosen register/constant
     */
    __asm__ __volatile__ ("inb %1, %0" : "=a" (ret) : "Nd" (port));
    // return the byte collected from hardware port.
    return ret;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "d"(port));
}
 6db4004 (Implement safe keyboard scan code filtering and temporary print buffers)
