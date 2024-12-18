/*** includes ***/

#include <ctype.h>
#include <errno.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <sys/ioctl.h>
#include <termios.h> 
#include <unistd.h>   
/*** defines ***/ 

#define CTRL_KEY(k) ((k) & 0x1f)
#define EDITOR_VERSION "0.0.1"
#define ABUF_INIT {NULL, 0}

/*** data ***/ 

struct editorConfig { 
    int cursor_x, cursor_y;
    int screenrows;
    int screencols;
    struct termios orig_termios;
}; 


struct editorConfig e_config;

/*** Append Buffer Structure ***/

struct abuf {
    char *c;
    int len;
};

void append(struct abuf *in_buffer, const char *s, int len) { 
    char *new_buffer = realloc(in_buffer->c, in_buffer->len + len);
    if(new_buffer == NULL) 
        return;
    memcpy(&new_buffer[in_buffer->len], s, len);
    in_buffer->c = new_buffer;
    in_buffer->len += len;
} 

void buffer_free(struct abuf *buffer) { 
    free(buffer->c); 
} 

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

void editor_draw_rows(struct abuf* buffer) { 
    int y;
    for(y = 0; y < e_config.screenrows; y++) { 
        if(y == e_config.screenrows / 3) { 
            char welcome[80];
            int welcome_len = snprintf(welcome, sizeof(welcome), 
                    "CEditor -version %s", EDITOR_VERSION);
            if(welcome_len > e_config.screencols) { 
                welcome_len = e_config.screencols;
            } 
            int padding = (e_config.screencols - welcome_len) / 2;
            if(padding) { 
                append(buffer, "~", 1);
                padding--;
            } 
            while(padding--){ 
                append(buffer, " ", 1);
            } 
            append(buffer, welcome, welcome_len);
        } else{ 
            append(buffer, "~", 1);
        } 
        append(buffer, "\x1b[K", 3); 
        if(y < e_config.screenrows - 1) {  
            append(buffer, "\r\n", 2);
        } 
    } 
} 

void editor_clear_screen(void) { 
    struct abuf buffer = ABUF_INIT;
    // hides the cursor 
    append(&buffer, "\x1b[25l", 6);
    append(&buffer, "\x1b[H", 3);
    editor_draw_rows(&buffer);
    append(&buffer, "\x1b[H", 3);
    append(&buffer, "\x1b[?25h", 6);
    write(STDOUT_FILENO, buffer.c, buffer.len);
    buffer_free(&buffer);
}

/*** init ***/

void init_editor(void) { 
    e_config.cursor_x = 0;
    e_config.cursor_y = 0;
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

