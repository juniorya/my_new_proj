### Configuration ###
CC 		= g++
CFLAGS 		= -Wall -g
#SOURCES 	= main.c application.c arch/linux.c radio/r_emu.c core/scheduler.c
#OBJECTS 	= $(SOURCES:.c=.o)

.PHONY:		main

%.o:	%.c
	$(CC) -c $(CC_FLAGS) $(INC_PARAMS) $< -o $@

all:		clearing clean soft

clearing:
	clear

clean:
	rm -f *.o

soft:		main.o
	$(CC) $(CFLAGS) $(LIB_PARAMS) -o parser $^
