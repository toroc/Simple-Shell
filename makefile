SOURCES.c= main.c simple_shell.c
INCLUDES= simple_shell.h
CFLAGS=
SLIBS=
PROGRAM = main

OBJECTS= $(SOURCES.c:.c=.o)
.KEEP_STATE:
debug := CFLAGS= -g
all debug: $(PROGRAM)

$(PROGRAM): $(INCLUDES) ($OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(SLIBS)

clean:
	rm -f $(PROGRAM) $(OBJECTS)
