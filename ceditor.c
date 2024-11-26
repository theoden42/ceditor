/*** includes ***/

#include <ctype.h>
#include <errno.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h> 
#include <unistd.h>   
/*** defines ***/ 

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/ 

struct editorConfig { 
    int screenrows;
    int screencols;
    struct termios orig_termios;
}; 

struct editorConfig e_config;

/*** terminal display ***/ 

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
} 

void disable_raw_mode(void){ 
   if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &e_config.orig_termios) == -1) { 
       die("tcsetattrerror");
   } 
} 

void enable_raw_mode(void){ 
    if(tcgetattr(STDIN_FILENO, &e_config.orig_termios) == -1) { 
        die("tcgetattrError");
    } 
    atexit(disable_raw_mode);

    struct termios raw_termios = e_config.orig_termios;
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

char editor_read_key(void){ 
    ssize_t nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){ 
        if(nread == -1 && errno != EAGAIN){
            die("Read Error"); 
        } 
    } 
    return c;
} 

int get_cursor_position(int *rows, int *cols) { 
    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) 
        return -1;
    while(i < sizeof(buf) - 1) { 
        if(read(STDIN_FILENO, &buf[i], 1) != 1){ 
            break;
        } else if(buf[i] == 'R') { 
            break;
        } 
        i++;
    } 
    buf[i] = '\0';
    if(buf[0] != '\x1b' || buf[1] != '[') { 
        return -1;
    } 
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) { 
        return -1;
    } 
    return 0;
} 

int get_window_size(int* cols, int* rows) { 
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){ 
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) { 
            return -1;
        } 
        return get_cursor_position(rows, cols);
    } else { 
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    } 
} 
    

/*** Input ***/ 

void editor_process_keypress(void) { 
    char c = editor_read_key(); 
    switch(c){ 
        case CTRL_KEY('q'): 
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    } 
} 

/*** Output ***/ 

void editor_draw_rows(void) { 
    int y;
    for(y = 0; y < e_config.screenrows; y++) { 
        write(STDOUT_FILENO, "~", 1);
        if(y < e_config.screenrows - 1) {  
            write(STDOUT_FILENO, "\r\n", 2);
        } 
    } 
} 

void editor_clear_screen(void) { 
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editor_draw_rows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** init ***/

void init_editor(void) { 
    if(get_window_size(&e_config.screenrows, &e_config.screencols) == -1) { 
        die("Failed to get window size");
    }
} 


int main(void) { 
    enable_raw_mode();
    init_editor();
    while(1){
        editor_clear_screen();
        editor_process_keypress();
        
    }

    return 0;
}

