CXX = g++
CXXFLAGS = -std=c++11 -I/opt/homebrew/Cellar/sdl2/2.32.4/include/SDL2
LDFLAGS = -L/opt/homebrew/Cellar/sdl2/2.32.4/lib -lSDL2

main: main.cpp
	$(CXX) main.cpp $(CXXFLAGS) $(LDFLAGS) -o mygame

