#include "include/shell.h"
#include "include/vga.h"
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
        vga_print("NG-OS v1.0\nAvailable commands:\nclear\nhelp\necho\n");
    }else if (strcmp(command, "clear") == 0) {}
    else if (strcmp(command, "echo") == 0) {
        //we start looping at 1, because args[0] is the word "echo" itself!
        for (int i = 1; i < num_args; i++) {
            vga_print(current_args[i]);
            vga_print(" "); //add the spaces back visually
        }
    }
    else{
        vga_print("Command not found: ");
        vga_print(command);
    }
}