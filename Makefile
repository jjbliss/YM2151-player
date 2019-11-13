OBJS = ym2151.o main.o

HEADERS = ym2151.h
SDL2CONFIG=sdl2-config
CFLAGS=-std=c99 -O3 -Wall -Werror -g $(shell $(SDL2CONFIG) --cflags)

LDFLAGS=$(shell $(SDL2CONFIG) --libs) -lm

OUTPUT=spplayer


all: $(OBJS) $(HEADERS)
	$(CC) -o $(OUTPUT) $(OBJS) $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: rm $(OUTPUT) *.o
