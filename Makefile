BIN	= sdfield4
OBJECTS	= sdfield4.o
CXX = clang

CXXFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES -g
LDFLAGS = -L/usr/X11R6/lib -g
LDLIBS  = -lGL -lglut -lm

$(BIN): $(OBJECTS)

clean:
	rm -rf $(BIN) $(OBJECTS)


