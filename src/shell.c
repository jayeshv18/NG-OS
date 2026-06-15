#include "include/shell.h"
#include "include/vga.h"
#include <stdint.h>
//we define the actual physical array here.
#define MAX_ARGS 16
char* current_args[MAX_ARGS];
/*
echo hello world and hit enter. shell_buffer looks like this:
[e][c][h][o][ ][h][e][l][l][o][ ][w][o][r][l][d][\0]
to pass these as separate arguments to a command, we don't want to copy them into new memory.
memory allocation is expensive! Instead, we do a trick called In-Place Mutation.
we scan the array. Every time we find a space (' '), we overwrite it with a null terminator ('\0').
then, we record the memory address of the very next letter.
after mutation, the exact same memory looks like this:
[e][c][h][o][\0][h][e][l][l][o][\0][w][o][r][l][d][\0]
 */
//returns the number of arguments found
int parse_args(char* input, char** args_array) {
    //initializing variables
    int arg_count =0;//create a variable to count how many arguments we've found
    int in_word=0;//create a state tracker
    int i=0;
    while (input[i] != '\0') { //loop that reads input[i] until it hits the final '\0'.
        if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n') {//no longer in a word.
            in_word=0; //change state
            input[i] = '\0';
        }
        else if (in_word==0) { //input[i] is NOT a space, AND are NOT currently in a word
            if (arg_count < MAX_ARGS) {//prevent overflowing our 16-argument array
                args_array[arg_count] = &input[i];//just found the first letter of a brand new word.
                arg_count++;
                in_word=1; //change state because now scanning inside a word
            }
        }
        i++;
    }
    return arg_count;//return total arguments found
}

int strcmp(const char *str1, const char *str2) {
    int i=0;
    while (str1[i] == str2[i]) {
        if (str1[i] == '\0') {
            return 0; //strings ended at the same time
        }
        i++;
    }
    return str1[i] - str2[i]; //difference when dont match
}

void execute_command(char* input) {
    //parse the raw string into our global array
    int num_args = parse_args(input, current_args);
    //the user just hit Enter (0 arguments), do nothing and return.
    if (num_args == 0) {
        return;
    }
    //args_array[0] is ALWAYS the main command (e.g., "echo", "help", "clear")
    char* command = current_args[0];
    if (strcmp(command, "clear") == 0) {
        vga_clear_screen();
    }
    else if (strcmp(command, "help") == 0) {
        vga_print("NG-OS v1.0\nAvailable commands:\nclear\nhelp\necho\npeek\n");
    }else if (strcmp(command, "clear") == 0) {}
    else if (strcmp(command, "echo") == 0) {
        //we start looping at 1, because args[0] is the word "echo" itself!
        for (int i = 1; i < num_args; i++) {
            vga_print(current_args[i]);
            vga_print(" "); //add the spaces back visually
        }
    }else if (strcmp(command, "peek") == 0) {
        if (num_args<2) {
            vga_print("Usage: peek <hex_address>\n");
            return;
        }
        //convert the text argument to a real integer
        uint32_t target_address = textohex(current_args[1]);
        //cast the integer into a 32-bit hardware pointer
        uint32_t* memory_pointer = (uint32_t*)target_address;
        //dereference the pointer to grab the physical RAM data
        uint32_t memory_contents = *memory_pointer;
        vga_print("Data at ");
        vga_print(current_args[1]);
        vga_print(": ");
        vga_hex_print(memory_contents);
        vga_print("\n");
    }
    else{
        vga_print("Command not found: ");
        vga_print(command);
    }
}

//takes a hex string from the terminal, translates it into a raw hardware pointer, and prints out exactly what is sitting in that physical RAM slot.
uint32_t textohex(const char* hex_str) {
    uint32_t result = 0;
    int i = 0;
    //skip the "0x" if typed it
    if (hex_str[0] == '0' && (hex_str[1] == 'x' || hex_str[1] == 'X')) {
        i = 2;
    }while (hex_str[i] != '\0') {
        char c = hex_str[i];
        uint32_t hex_val = 0;
        if (c >= '0' && c <= '9') {
            hex_val = c - '0'; //0 hold 48 in ascii, c - '0', the computer replaces those characters with their secret ASCII numbers and subtracts them
        }
        else if (c >= 'A' && c <= 'F') {
            //letter 'A' represents the number 10, 'B' is 11, and so on up to 'F', which is 15
            //in the ASCII table:'A' is 65, 'B' is 66, 'F' is 70
            //if c is 'C':'C' is 67 in ASCII. hex_val = 67 - 65 + 10, hex_val = 2 + 10 = 12
            //the letter 'C' correctly becomes the number 12
            hex_val = c-'A' + 10;
        }
        else if (c >= 'a' && c <= 'f') {
            hex_val = c-'a' + 10;
        }
        else {
            // invalid character! Just break the loop.
            break;
        }
        //shift the result left by 1 hex digit (4 bits) and add the new value
        result=(result<<4)|hex_val;
        i++;
    }
    return result;
}