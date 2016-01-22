#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

struct vec2 {
	float x, y;
	vec2() : x(0.0f), y(0.0f) {}
};

struct panel_t;
struct panel_t {
	float x, y, width, height;
	uint32_t color;
	panel_t *prev;
	panel_t *next;
	void *userdata;
	void (*handler)(SDL_Event*, panel_t*);
};

panel_t *rootPanel = NULL;
panel_t *endPanel = NULL;

void handler1(SDL_Event *evt, panel_t *panel) {
	printf("handler 1\n");
}

void handler2(SDL_Event *evt, panel_t *panel) {
	printf("handler 2\n");
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

bool eventIsPositional(SDL_Event *evt, vec2 *outPosition) {
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
	rootPanel->color = SDL_MapRGB(surface->format, 0xFF, 0x00, 0x00);
	rootPanel->handler = handler1;

	rootPanel->next = (panel_t*) malloc(sizeof(panel_t));
	rootPanel->next->x = 470;
	rootPanel->next->y = 100;
	rootPanel->next->width = 300;
	rootPanel->next->height = 300;
	rootPanel->next->prev = rootPanel;
	rootPanel->next->next = NULL;
	rootPanel->next->color = SDL_MapRGB(surface->format, 0x00, 0xFF, 0x00);
	rootPanel->next->handler = handler2;

	endPanel = rootPanel->next;
	
	render(surface);
	SDL_UpdateWindowSurface(window);

	SDL_Event evt;
	while (1) {
		SDL_WaitEvent(&evt);
		if (evt.type == SDL_WINDOWEVENT && evt.window.event == SDL_WINDOWEVENT_CLOSE) {
			break;
		}

		vec2 eventPos;
		if (eventIsPositional(&evt, &eventPos)) {
			panel_t *eventPanel = findPanelAtPoint(eventPos);
			if (eventPanel) {
				eventPanel->handler(&evt, eventPanel);
			}
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}