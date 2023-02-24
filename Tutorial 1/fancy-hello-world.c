#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fancy-hello-world.h"

#define MAX_NAME_SIZE 100
#define HELLO_PREFIX "Hello World, hello "

int main() {
    // Reads in name from stdin
    char* name = (char*) malloc(MAX_NAME_SIZE * sizeof(char));
    name = fgets(name, MAX_NAME_SIZE, stdin);
    if (name == NULL)
        return -1;

    // Determines size of the final "hello, world" message (+1 for null terminator)
    int hello_size = strlen(name) + strlen(HELLO_PREFIX) + 1;
    
    // Allocates space for string
    char* output = (char*) malloc(hello_size * sizeof(char));

    // Builds string
    hello_string(name, output);

    // Prints string
    printf("%s", output);

    // Frees memory
    free(name); 
    free(output);

    return 0;
}

void hello_string(char* name, char* output) {
    // Prefix for message
    const char* prefix = HELLO_PREFIX;

    // Inserts null terminator (necessary for first 'strcat' call)
    output[0] = '\x00';

    // Builds output string
    strcat(output, prefix);
    strcat(output, name);
}