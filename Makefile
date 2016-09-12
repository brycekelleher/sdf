BIN	= sdfield6
OBJECTS	= sdfield6.o
CXX = clang

CXXFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES -Wall
LDFLAGS = -L/usr/X11R6/lib
LDLIBS  = -lGL -lglut -lm

$(BIN): $(OBJECTS)

clean:
	rm -rf $(BIN) $(OBJECTS)


