/*
 * READY: The Retro Computing Environment You Never Asked For
 * Copyright (C) 2016  Jason Frame <jason@onehackoranother.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "keymap.qwerty.inc.cpp"

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

void blitChar(SDL_Surface *surface, font_t *font, char ch, int x, int y) {
	SDL_Rect srcRect, destRect;
	srcRect.x = (ch & 0x1F) * font->charWidth;
	srcRect.y = (ch >> 5) * font->charHeight;
	destRect.x = x;
	destRect.y = y;
	srcRect.w = destRect.w = font->charWidth;
	srcRect.h = destRect.h = font->charHeight;
	SDL_BlitSurface(font->surface, &srcRect, surface, &destRect);
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
	void (*render)(SDL_Surface*, SDL_Rect*, panel_t*);
};

font_t systemFont;

panel_t *rootPanel = NULL;
panel_t *endPanel = NULL;
panel_t *keyPanel = NULL;

void makeKeyPanel(panel_t *panel) {
	keyPanel = panel;
}

const int CONSOLE_HISTORY = 512;

struct console_line_t {
	int length; // number of characters, excluding null terminator
	int promptLength;
	int pixelHeight; // computed based on panel width, character height and line length
	char *buffer;
	int bufferSize;
};

struct console_state_t {
	int maxLines, startLine, endLine;
	bool empty;
	console_line_t lines[CONSOLE_HISTORY];
};

void consoleBackwardsDeleteChar(console_state_t *console) {

}

void consoleGrowBuffer(console_line_t *line, int dlen) {
	int requiredSize = line->length + dlen + 1;
	if (line->bufferSize < requiredSize) {
		while (line->bufferSize < requiredSize) {
			line->bufferSize *= 2;
		}
		line->buffer = (char*)realloc(line->buffer, line->bufferSize);
		// TODO: check for failure
	}
}

void consoleAppend(console_state_t *console, char ch) {
	console_line_t *line = &console->lines[console->endLine];
	consoleGrowBuffer(line, 1);
	line->buffer[line->length++] = ch;
	line->buffer[line->length] = 0;
}

void consoleAppend(console_state_t *console, const char *str) {
	console_line_t *line = &console->lines[console->endLine];
	consoleGrowBuffer(line, strlen(str));
	while (*str) {
		line->buffer[line->length++] = *(str++);
	}
	line->buffer[line->length] = 0;
}

void consoleStartLine(console_state_t *console, const char *prompt = NULL) {
	if (console->empty) {
		console->empty = false;
	} else {
		console->endLine = (console->endLine + 1) % console->maxLines;
		if (console->endLine == console->startLine) {
			free(console->lines[console->endLine].buffer);
			console->startLine = (console->startLine + 1) % console->maxLines;
		}
	}
	int line = console->endLine;
	console->lines[line].length = 0;
	console->lines[line].buffer = (char*)malloc(128);
	// TODO: check for failure
	console->lines[line].buffer[0] = 0;
	console->lines[line].bufferSize = 128;
	if (prompt != NULL) {
		consoleAppend(console, prompt);
		console->lines[line].promptLength = strlen(prompt);
	} else {
		console->lines[line].promptLength = 0;
	}
}

void consoleInit(console_state_t *console) {
	console->maxLines = CONSOLE_HISTORY;
	console->startLine = 0;
	console->endLine = 0;
	console->empty = true;
	consoleStartLine(console, "READY");
	consoleStartLine(console, "> ");
}

void consolePanelHandler(SDL_Event *evt, panel_t *panel) {
	console_state_t *console = (console_state_t*)panel->userdata;
	if (evt->type == SDL_MOUSEBUTTONDOWN) {
		makeKeyPanel(panel);
	}
	if (evt->type == SDL_KEYDOWN) {
		SDL_Scancode sc = evt->key.keysym.scancode;
		Uint16 mod = evt->key.keysym.mod;
		char ch = 0;
		if (sc < 256) {
			if (mod & KMOD_SHIFT) {
				ch = keyCharShiftMap[sc];
			} else {
				ch = keyCharMap[sc];
			}
			if (mod & KMOD_CAPS) {
				if (ch >= 65 && ch <= 90) {
					ch += 32;
				} else if (ch >= 97 && ch <= 122) {
					ch -= 32;
				}
			}
		} else {
			printf("weird scan code: %d\n", sc);
		}
		if (ch > 0) {
			consoleAppend(console, ch);
		} else {
			SDL_Keycode sym = evt->key.keysym.sym;
			if (sym == SDLK_BACKSPACE) {
				consoleBackwardsDeleteChar(console);
			} else if (sym == SDLK_RETURN) {
				consoleStartLine(console, "> ");
			}
		}
	}
}

void consolePanelRender(SDL_Surface *surface, SDL_Rect *rect, panel_t *panel) {
	console_state_t *console = (console_state_t*)panel->userdata;
	
	SDL_FillRect(surface, rect, panel->color);
	
	if (console->empty) {
		return;
	}

	int padding = 10;
	int availableWidth = panel->width - (padding * 2);
	int charsWide = availableWidth / (systemFont.charWidth);
	int startX = panel->x + padding;
	int maxX = startX + (charsWide * systemFont.charWidth);
	int currY = panel->y + padding;
	int lineIndex = console->startLine;

	while (true) {
		console_line_t *line = &console->lines[lineIndex];
		int currX = startX;
		for (int i = 0; i < line->length; ++i) {
			blitChar(surface, &systemFont, line->buffer[i], currX, currY);
			currX += systemFont.charWidth;
			if (currX >= maxX) {
				currX = startX;
				currY += systemFont.charHeight;
			}
		}
		currY += systemFont.charHeight;
		if (lineIndex == console->endLine) {
			break;
		}
		lineIndex = (lineIndex + 1) % console->maxLines;
	}
}

void handler1(SDL_Event *evt, panel_t *panel) {
	//printf("handler %p\n", panel->userdata);
	if (evt->type == SDL_MOUSEBUTTONDOWN) {
		makeKeyPanel(panel);
	}
}

void renderColorBox(SDL_Surface *surface, SDL_Rect *rect, panel_t *panel) {
	SDL_FillRect(surface, rect, panel->color);
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
		SDL_SetClipRect(surface, &destRect);
		p->render(surface, &destRect, p);
		p = p->next;
	}

	SDL_SetClipRect(surface, NULL);
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

	const int windowWidth = 800;
	const int windowHeight = 600;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	atexit(SDL_Quit);

	SDL_Window *window = SDL_CreateWindow("READY", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);
	SDL_Surface *surface = SDL_GetWindowSurface(window);

	console_state_t consoleState;
	consoleInit(&consoleState);

	console_state_t consoleState2;
	consoleInit(&consoleState2);

	rootPanel = (panel_t*) malloc(sizeof(panel_t));
	rootPanel->x = 0;
	rootPanel->y = 0;
	rootPanel->width = windowWidth / 2;
	rootPanel->height = windowHeight;
	rootPanel->prev = NULL;
	rootPanel->color = SDL_MapRGB(surface->format, 0x01, 0x00, 0x7F);
	rootPanel->handler = consolePanelHandler;
	rootPanel->render = consolePanelRender;
	rootPanel->userdata = (void*)(&consoleState);

	rootPanel->next = (panel_t*) malloc(sizeof(panel_t));
	rootPanel->next->x = windowWidth / 2;
	rootPanel->next->y = 0;
	rootPanel->next->width = windowWidth / 2;
	rootPanel->next->height = windowHeight;
	rootPanel->next->prev = rootPanel;
	rootPanel->next->next = NULL;
	rootPanel->next->color = SDL_MapRGB(surface->format, 0x00, 0xFF, 0x00);
	rootPanel->next->handler = consolePanelHandler;
	rootPanel->next->render = consolePanelRender;
	rootPanel->next->userdata = (void*)&consoleState2;

	endPanel = rootPanel->next;

	loadFont("fonts/FJG.gif", &systemFont, 0xFF, 0xFF, 0xFF);

	blitString(surface, &systemFont, "Hello World\nREADY", 10, 10, 0xff, 0xff, 0x0b);
	
	while (1) {
		SDL_Event evt;
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE) {
				goto exit;
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
		render(surface);
		// blitString(surface, &systemFont, "> (+ 20 30)\n 50", 10, 42, 0xff, 0xff, 0x00);
		SDL_UpdateWindowSurface(window);
		// TODO: proper timing loop
		SDL_Delay(33);
	}

exit:

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}