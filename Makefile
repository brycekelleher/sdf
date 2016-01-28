OBJECTS	= sdfield3.o
CXX = clang

CFLAGS += -I/usr/X11R6/include -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib
LDLIBS  = -lGL -lglut -lm

sdfield3: $(OBJECTS)

clean:
	rm -rf sdfield3 $(OBJECTS)


