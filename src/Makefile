
EXE = ../bin/debugger
IMGUI_DIR = ../vendor/imgui

LIBS = -lGL `pkg-config --static --libs glfw3`
CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends `pkg-config --cflags glfw3`

all:
	g++ $(CXXFLAGS) main.cpp -o $(EXE) $(LIBS)
#make console
#make elfdump

console: debugger.cpp
	g++ debugger.cpp -o debugger

elfdump: elfdump.cpp PrintData.h PrintData.cpp
	g++ elfdump.cpp -o elfdump

clean:
	rm -f $(EXE)
	rm -f debugger
	rm -f elfdump
