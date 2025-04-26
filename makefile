CXX = g++
CXXFLAGS = -std=c++11 -I/opt/homebrew/include/SDL2
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lSDL2_mixer -lSDL2_ttf -lSDL2_gfx -lm

main: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp $(LDFLAGS) -o mygame

