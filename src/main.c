#include <stdio.h>


#ifndef VERSION
#define VERSION "unknown"
#endif


int main(int argc, char *argv[]){
    printf("%s", VERSION);
}