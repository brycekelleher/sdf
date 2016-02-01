BIN	= sdfield4
OBJECTS	= sdfield4.o
CXX = clang

CXXFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib
LDLIBS  = -lGL -lglut -lm

$(BIN): $(OBJECTS)

clean:
	rm -rf $(BIN) $(OBJECTS)


