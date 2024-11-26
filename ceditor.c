#include <ctype.h>
#include <unistd.h>   
#include <termios.h> 
#include <stdlib.h>
#include <stdio.h> 

struct termios orig_termios;

void disable_raw_mode(void){ 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
} 

void enable_raw_mode(void){ 
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw_termios = orig_termios;
    raw_termios.c_iflag &= ~(ICRNL | IXON); 
    raw_termios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw_termios.c_oflag &= ~(OPOST);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios);
} 

int main(void) { 
    enable_raw_mode();
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){ 
        if(iscntrl(c)) { 
            printf("char: %d \r\n", c);
        } else {
            printf("char: %d(%c) \r\n", c, c);
        } 
    }
    return 0;
}

