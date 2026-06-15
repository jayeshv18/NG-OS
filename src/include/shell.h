//
// Created by jay on 6/15/26.
//

#ifndef NGOS_SHELL_H
#define NGOS_SHELL_H
//the maximum number of arguments a command can have
#define MAX_ARGS 16

//we declare our global array here using 'extern' so ANY file that
// includes shell.h can access the parsed arguments!
extern char* current_args[MAX_ARGS];

// function prototypes (The Menu)
int parse_arguments(char* input, char** args_array);
void execute_command(char* input);
int strcmp(const char *str1, const char *str2);
#endif
