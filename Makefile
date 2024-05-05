UNAME := $(shell uname -s)
ifeq ($(UNAME), Linux)
	CFLAGS = -lSDL2
endif
ifeq ($(UNAME), Darwin)
	CFLAGS = -F/Library/Frameworks -framework SDL2
endif

all: list.c view.c model.c controller.c
	gcc list.c view.c model.c controller.c $(CFLAGS) -lm

clean:
	rm -f *.o
