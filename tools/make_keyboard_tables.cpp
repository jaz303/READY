#include <SDL2/SDL.h>

#define keycase(v, n) case v: printf("    '%c',\n", n); break

int main(int argc, char *argv[]) {

    SDL_Init(SDL_INIT_EVERYTHING);
    
    printf("const int keyCharMap[] = {\n");
    for (int i = 0; i < 256; ++i) {
        SDL_Keycode k = SDL_GetKeyFromScancode((SDL_Scancode)i);
        if ((k >= 'a' && k <= 'z') || (k >= '0' && k <= '9')) {
            printf("    '%c',\n", k);
        } else {
            switch (k) {
                case '\\': printf("    '\\\\',\n"); break;
                case '\'': printf("    '\\\'',\n"); break;
                keycase(' ', ' ');
                keycase('-', '-');
                keycase('=', '=');
                keycase('[', '[');
                keycase(']', ']');
                keycase(';', ';');
                keycase('#', '#');
                keycase(',', ',');
                keycase('.', '.');
                keycase('/', '/');
                default: printf("    0,\n"); break;
            }
        }
    }
    printf("};\n\n");

    printf("const int keyCharShiftMap[] = {\n");
    for (int i = 0; i < 256; ++i) {
        SDL_Keycode k = SDL_GetKeyFromScancode((SDL_Scancode)i);
        if (k >= 'a' && k <= 'z') {
            printf("    '%c',\n", k - 32);
        } else {
            switch (k) {
                keycase('1', '!');
                keycase('2', '"');
                keycase('3', '?');
                keycase('4', '$');
                keycase('5', '%');
                keycase('6', '^');
                keycase('7', '&');
                keycase('8', '*');
                keycase('9', '(');
                keycase('0', ')');
                keycase(' ', ' ');
                keycase('\\', '|');
                keycase('-', '_');
                keycase('=', '+');
                keycase('[', '{');
                keycase(']', '}');
                keycase(';', ':');
                keycase('\'', '@');
                keycase('#', '~');
                keycase(',', '<');
                keycase('.', '>');
                keycase('/', '?');
                default: printf("    0,\n"); break;
            }
        }
    }
    printf("};\n\n");

    // printf("const int keyActionMap[] = {\n");
    // for (int i = 0; i < 256; ++i) {
    //  SDL_Keycode k = SDL_GetKeyFromScancode((SDL_Scancode)i);
    //  const char *name = NULL;

    //  switch (k) {
    //      namecase(RETURN, "return");
    //      namecase(BACKSPACE, "backspace");
    //      namecase(ESCAPE, "escape");
    //      namecase(F1, "F1");
    //      namecase(F2, "F2");
    //      namecase(F3, "F3");
    //      namecase(F4, "F4");
    //      namecase(F5, "F5");
    //      namecase(F6, "F6");
    //      namecase(F7, "F7");
    //      namecase(F8, "F8");
    //      namecase(F9, "F9");
    //      namecase(F10, "F10");
    //      namecase(F11, "F11");
    //      namecase(F12, "F12");
    //  }

    //  if (name) {
    //      printf("    0, // %s\n", name);
    //  } else {
    //      printf("    0,\n");
    //  }
    // }
    // printf("};\n\n");
    
    return 0;
}