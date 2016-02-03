#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef float real;

struct vec2 {
	real x, y;
	vec2() : x(0.0f), y(0.0f) {}
};

struct font_t {
	SDL_Surface *surface;
	int charWidth, charHeight;
	int rows, columns;
	SDL_Color *bgColor;
	SDL_Color *fgColor;
};

// fonts are, for now, 2-color images with indexed pixel formats
bool loadFont(const char *filename, font_t *font, Uint8 r, Uint8 g, Uint8 b) {
	font->surface = IMG_Load(filename);
	if (font->surface == NULL) {
		return false;
	}
	
	printf("loaded font %s, pixel format = %s\n", filename, SDL_GetPixelFormatName(font->surface->format->format));
	printf("colors in palette = %d\n", font->surface->format->palette->ncolors);

	// find the background colour and set it as the colour key
	Uint32 bgIndex = SDL_MapRGB(font->surface->format, r, g, b);
	SDL_SetColorKey(font->surface, SDL_TRUE, bgIndex);
	printf("background index = %d\n", bgIndex);

	// store a pointer to the foreground colour's palette entry so we can update it when we blit the string
	font->fgColor = &font->surface->format->palette->colors[bgIndex == 1 ? 0 : 1];

	// assume 32x8 chars
	font->charWidth = font->surface->w / 32;
	font->charHeight = font->surface->h / 8;

	return true;
}

void blitString(SDL_Surface *surface, font_t *font, const char *str, int x, int y, int r = 255, int g = 255, int b = 255) {
	font->fgColor->r = r;
	font->fgColor->g = g;
	font->fgColor->b = b;
	SDL_Rect srcRect, destRect;
	destRect.x = x;
	destRect.y = y;
	srcRect.w = destRect.w = font->charWidth;
	srcRect.h = destRect.h = font->charHeight;
	while (*str) {
		if (*str == '\n') {
			destRect.x = x;
			destRect.y += font->charHeight;
		} else {
			srcRect.x = (*str & 0x1F) * font->charWidth;
			srcRect.y = (*str >> 5) * font->charHeight;
			SDL_BlitSurface(font->surface, &srcRect, surface, &destRect);
			destRect.x += font->charWidth;
		}
		str++;
	}
}

struct panel_t;
struct panel_t {
	real x, y, width, height;
	uint32_t color;
	panel_t *prev;
	panel_t *next;
	void *userdata;
	void (*handler)(SDL_Event*, panel_t*);
};

font_t systemFont;

panel_t *rootPanel = NULL;
panel_t *endPanel = NULL;
panel_t *keyPanel = NULL;

void makeKeyPanel(panel_t *panel) {
	keyPanel = panel;
}

void handler1(SDL_Event *evt, panel_t *panel) {
	//printf("handler %p\n", panel->userdata);
	if (evt->type == SDL_MOUSEBUTTONDOWN) {
		makeKeyPanel(panel);
	}
}

void render(SDL_Surface *surface) {
	SDL_Rect destRect;
	destRect.x = 0;
	destRect.y = 0;
	destRect.w = surface->w;
	destRect.h = surface->h;

	SDL_FillRect(surface, &destRect, SDL_MapRGB(surface->format, 0, 0, 0));
	
	panel_t *p = rootPanel;
	while (p) {
		destRect.x = p->x;
		destRect.y = p->y;
		destRect.w = p->width;
		destRect.h = p->height;
		SDL_FillRect(surface, &destRect, p->color);
		p = p->next;
	}
}

bool eventIsSpatial(SDL_Event *evt, vec2 *outPosition) {
	switch (evt->type) {
		case SDL_MOUSEMOTION:
			outPosition->x = evt->motion.x;
			outPosition->y = evt->motion.y;
			return true;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			outPosition->x = evt->button.x;
			outPosition->y = evt->button.y;
			return true;
		case SDL_MOUSEWHEEL:
			outPosition->x = evt->wheel.x;
			outPosition->y = evt->wheel.y;
			return true;
	}
	return false;
}

bool eventIsKeyboard(SDL_Event *evt) {
	return evt->type == SDL_KEYDOWN
			|| evt->type == SDL_KEYUP
			|| evt->type == SDL_TEXTEDITING
			|| evt->type == SDL_TEXTINPUT;
}

panel_t* findPanelAtPoint(vec2 point) {
	panel_t *p = endPanel;
	while (p) {
		if (point.x >= p->x && point.y >= p->y && point.x < (p->x + p->width) && point.y < (p->y + p->height)) {
			return p;
		}
		p = p->prev;
	}
	return NULL;
}

int main(int argc, char *argv[]) {

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	atexit(SDL_Quit);

	SDL_Window *window = SDL_CreateWindow("READY", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, 0);
	SDL_Surface *surface = SDL_GetWindowSurface(window);

	rootPanel = (panel_t*) malloc(sizeof(panel_t));
	rootPanel->x = 10;
	rootPanel->y = 10;
	rootPanel->width = 500;
	rootPanel->height = 600;
	rootPanel->prev = NULL;
	rootPanel->color = SDL_MapRGB(surface->format, 0x01, 0x00, 0x7F);
	rootPanel->handler = handler1;
	rootPanel->userdata = (void*)0x01;

	rootPanel->next = (panel_t*) malloc(sizeof(panel_t));
	rootPanel->next->x = 470;
	rootPanel->next->y = 100;
	rootPanel->next->width = 300;
	rootPanel->next->height = 300;
	rootPanel->next->prev = rootPanel;
	rootPanel->next->next = NULL;
	rootPanel->next->color = SDL_MapRGB(surface->format, 0x00, 0xFF, 0x00);
	rootPanel->next->handler = handler1;
	rootPanel->next->userdata = (void*)0x02;

	endPanel = rootPanel->next;

	loadFont("fonts/FJG.gif", &systemFont, 0xFF, 0xFF, 0xFF);
	
	render(surface);

	blitString(surface, &systemFont, "Hello World\nREADY", 10, 10, 0xff, 0xff, 0x0b);
	blitString(surface, &systemFont, "> (+ 20 30)\n 50", 10, 42, 0xff, 0xff, 0x00);

	SDL_UpdateWindowSurface(window);

	SDL_Event evt;
	while (1) {
		SDL_WaitEvent(&evt);
		if (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE) {
			break;
		}

		vec2 eventPos;
		if (eventIsSpatial(&evt, &eventPos)) {
			panel_t *eventPanel = findPanelAtPoint(eventPos);
			if (eventPanel) {
				eventPanel->handler(&evt, eventPanel);
			}
		} else if (keyPanel && eventIsKeyboard(&evt)) {
			keyPanel->handler(&evt, keyPanel);
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}