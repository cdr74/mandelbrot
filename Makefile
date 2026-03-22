# Direct MinGW build — reuses prebuilt deps from 3-body-problem
DEPS = /home/chris/dev/3-body-problem/build-windows

CXX = x86_64-w64-mingw32-g++
CC  = x86_64-w64-mingw32-gcc

OUTDIR = build-windows/bin
OBJ    = build-windows/obj

INCLUDES = \
  -I$(DEPS)/_deps/glfw-src/include \
  -I$(DEPS)/_deps/imgui-src \
  -I$(DEPS)/_deps/imgui-src/backends \
  -I$(DEPS)/_deps/glm-src \
  -Iexternal/glad/include \
  -Isrc

LIBS = \
  $(DEPS)/lib/libimgui.a \
  $(DEPS)/lib/libglfw3.a \
  $(DEPS)/lib/libglad.a \
  -lopengl32 -lgdi32 -lcomdlg32 -lole32 -lshell32

CXXFLAGS = -O2 -std=c++17 -DNDEBUG $(INCLUDES)
LDFLAGS  = -static -static-libgcc -static-libstdc++ -mwindows

SRCS = src/main.cpp src/mandelbrot.cpp src/renderer.cpp src/ui.cpp
GLAD_SRC = external/glad/glad.c

OBJS = $(SRCS:src/%.cpp=$(OBJ)/%.o) $(OBJ)/glad.o

EXE = $(OUTDIR)/MandelbrotExplorer.exe

.PHONY: all clean

all: $(OUTDIR) $(OBJ) $(EXE)
	@cp -r shaders $(OUTDIR)/
	@echo ""
	@echo "Build successful!"
	@ls -lh $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ)/glad.o: external/glad/glad.c
	$(CC) -O2 -Iexternal/glad/include -c -o $@ $<

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OBJ):
	mkdir -p $(OBJ)

clean:
	rm -rf build-windows
