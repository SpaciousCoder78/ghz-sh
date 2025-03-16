

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define STEX_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1F)

struct erow {
    int size;
    char *chars;
};

struct editorConfiguration {
    int cx, cy;
    int rowOffset;
    int screencols;
    int screenrows;
    int numRows;
    struct erow *row;
    struct termios original_termios;
};

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

struct editorConfiguration E;
char *editorFilename = NULL;

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL)
        return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

int keyRead() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return 'H';
                        case '3': return 127;
                        case '4': return 'E';
                        case '5': return 'U';
                        case '6': return 'D';
                        case '7': return 'H';
                        case '8': return 'E';
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return 1000 + 0;
                    case 'B': return 1000 + 1;
                    case 'C': return 1000 + 2;
                    case 'D': return 1000 + 3;
                    case 'H': return 'H';
                    case 'F': return 'E';
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return 'H';
                case 'F': return 'E';
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

void exitRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1)
        die("tcsetattr");
}

void enterRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1)
        die("tcgetattr");
    atexit(exitRawMode);

    struct termios raw = E.original_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;
    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void editorAppendRow(char *s, size_t len) {
    E.row = realloc(E.row, sizeof(struct erow) * (E.numRows + 1));
    int at = E.numRows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numRows++;
}

void editorFileOpen(char *filename) {
    editorFilename = strdup(filename);
    FILE *f = fopen(filename, "r");
    if (!f) {
        if (errno == ENOENT) {
            // File not found -> initialize with an empty file
            return;
        }
        die("fopen");
    }
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, f)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(f);
}

void editorSave() {
    if (editorFilename == NULL)
        return;
    FILE *fp = fopen(editorFilename, "w");
    if (!fp)
        die("fopen");
    for (int i = 0; i < E.numRows; i++) {
        fwrite(E.row[i].chars, 1, E.row[i].size, fp);
        fputc('\n', fp);
    }
    fclose(fp);
}

void editorMoveCursor(int key) {
    switch (key) {
        case 1000 + 0:
            if (E.cy != 0)
                E.cy--;
            break;
        case 1000 + 1:
            if (E.cy < E.numRows - 1)
                E.cy++;
            break;
        case 1000 + 2:
            if (E.cx < E.screencols - 1)
                E.cx++;
            break;
        case 1000 + 3:
            if (E.cx != 0)
                E.cx--;
            break;
        default:
            break;
    }
}

void editorInsertChar(int c) {
    // If current row does not exist, create an empty row.
    if (E.cy == E.numRows)
        editorAppendRow("", 0);
    struct erow *row = &E.row[E.cy];
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[E.cx + 1], &row->chars[E.cx], row->size - E.cx + 1);
    row->chars[E.cx] = c;
    row->size++;
    E.cx++;
}

void editorKeyPress() {
    int c = keyRead();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
        case CTRL_KEY('s'):
            editorSave();
            break;
        case 'H':
            E.cx = 0;
            break;
        case 'E':
            E.cx = E.screencols - 1;
            break;
        case 127:
            break;
        case 'U':
        case 'D': {
            int times = E.screenrows;
            while (times--)
                editorMoveCursor(c == 'U' ? 1000 + 0 : 1000 + 1);
            break;
        }
        case 1000 + 0:
        case 1000 + 1:
        case 1000 + 2:
        case 1000 + 3:
            editorMoveCursor(c);
            break;
        default:
            if (isprint(c))
                editorInsertChar(c);
            break;
    }
}

void verticalScroll() {
    if (E.cy < E.rowOffset)
        E.rowOffset = E.cy;
    if (E.cy >= E.rowOffset + E.screenrows)
        E.rowOffset = E.cy - E.screenrows + 1;
}

void editorDrawRows(struct abuf *ab) {
    for (int y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowOffset;
        if (filerow >= E.numRows) {
            if (E.numRows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Stex editor -- version %s", STEX_VERSION);
                if (welcomelen > E.screencols)
                    welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "\x1b[0m~", 5);
                    padding--;
                }
                while (padding--)
                    abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].size;
            if (len > E.screencols)
                len = E.screencols;
            abAppend(ab, E.row[filerow].chars, len);
        }
        abAppend(ab, "\x1b[K", 3);
        if (y < E.screenrows - 1)
            abAppend(ab, "\r\n", 2);
    }
}

void editorRefreshScreen() {
    verticalScroll();
    struct abuf ab = ABUF_INIT;
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);
    editorDrawRows(&ab);
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy - E.rowOffset + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));
    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rowOffset = 0;
    E.numRows = 0;
    E.row = NULL;
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
}

