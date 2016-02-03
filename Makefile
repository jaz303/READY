main: main.cpp
	g++ -lSDL2 -lSDL2_image -lcairo -o $@ $<

clean:
	rm -f main

.PHONY: clean