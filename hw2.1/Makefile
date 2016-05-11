CC  = gcc
CXX = g++

INCLUDES = -I/usr/local/include/OpenEXR -I/usr/include/OpenEXR -I./headerfiles

CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES) 

CLICFLAGS = -L /usr/lib/x86_64-linux-gnu/ -Wl,--no-as-needed -lpthread -lglut -lGL -lGLU -lGLEW -lm

OSXFLAGS = -framework OpenGL -framework GLUT

NOTE = LD_PRELOAD=/lib/x86_64-linx-gnu/libpthread.so.0

objects = hello_triangle_transform.o initshader.o

hw = hw2.1

$(hw): $(objects)
	$(CXX) $(objects) $(OSXFLAGS) -g -O2 -o glrender

osx:
	$(CXX) *.cc $(OSXFLAGS) -o glrender

clic:	
	$(CXX) -g *.cc $(CLICFLAGS) -o glrender	-std=c++11

$(objects): amath.h checkerror.h mat.h vec.h

.PHONY: clean
clean:
	rm -f *~ a.out core *.o main *.dSYM glrender $(hw).exr

.PHONY: all
all: clean main