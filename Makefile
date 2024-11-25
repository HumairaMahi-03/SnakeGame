all:
	g++ -I src/include -L src/lib -o main main.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer
	./main
	g++ -I src/include -L src/lib -o snake snake.cpp -lmingw32 -lSDL2snake -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer
	./main