/*** includes ***/

#include <ctype.h>
#include <errno.h> 
#include <unistd.h>   
#include <termios.h> 
#include <stdlib.h>
#include <stdio.h> 

/*** defines ***/ 

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/ 

struct termios orig_termios;

/*** terminal display ***/ 

void die(const char *s) {
    perror(s);
    exit(1);
} 

void disable_raw_mode(void){ 
   if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) { 
       die("tcsetattrerror");
   } 
} 

void enable_raw_mode(void){ 
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) { 
        die("tcgetattrError");
    } 
    atexit(disable_raw_mode);

    struct termios raw_termios = orig_termios;
    raw_termios.c_iflag &= ~(ICRNL | IXON | BRKINT | ISTRIP | INPCK); 
    raw_termios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw_termios.c_cflag |= (CS8);
    raw_termios.c_oflag &= ~(OPOST);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1){ 
        die("tcsetattrError");
    } 
} 

/*** init ***/

int main(void) { 
    enable_raw_mode();
    while(1){
        char c;
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN){ 
            die("ReadError");
        } 
        if(iscntrl(c)) { 
            printf("char: %d \r\n", c);
        } else {
            printf("char: %d(%c) \r\n", c, c);
        } 
        if(c == CTRL_KEY('q')){ 
            break;
        } 
    }

    return 0;
}

