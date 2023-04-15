/* compiled with -ffreestanding in gcc */


/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

/* the VGA framebuffer starts at 0xb8000 */
#define VIDEO_MEMORY 0xb8000


/* this function writes a single character out to the screen */
void kprint(char *c, int line, int column, int color) {
    /* the VGA framebuffer consists of 25 lines each of 80 columns; each element takes 2 bytes */
    char *vidmem = (char *)VIDEO_MEMORY;
    int offset = BYTES_FOR_EACH_ELEMENT * (COLUMNS_IN_LINE * line + column);
    vidmem[offset] = *c;
    vidmem[offset + 1] = color; /* color is an attribute byte */
}

/* this function clears the screen */
void kclear() {
    char *vidmem = (char *)VIDEO_MEMORY;
    int i;
    for (i = 0; i < SCREENSIZE; i++) {
        vidmem[i] = 0;
    }
}

/* this function prints a string to the screen */
void kprint_string(char *s, int startLine, int startColumn, int color) {
    int i = 0;
    while (s[i] != '\0') {
        kprint(&s[i], startLine, startColumn + i, color);
        i++;
    }
}

void kmain() {
    kclear();
    kprint_string("Hello, World!", 0, 0, 0x0f);
}
