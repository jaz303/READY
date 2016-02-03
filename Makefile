main: main.cpp
	g++ -lSDL2 -lSDL2_image -o $@ $<

clean:
	rm -f main

.PHONY: clean