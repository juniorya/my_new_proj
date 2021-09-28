### Configuration ###
INC=. libs/libmicrohttpd-0.9.73/src/include
INC_PARAMS=$(foreach d, $(INC), -I$d)
LIB_PARAMS=-Llibs/libmicrohttpd-0.9.73/src/microhttpd/.libs
CC 		= g++
C_C 		= gcc
CFLAGS 		= -Wall -g
#SOURCES 	= main.cpp Manager.cpp billcache.cpp medias/*.cpp frontend/*.cpp arch/PosixLocker.cpp arch/PosixThread.cpp smart/*.cpp vendor/*.cpp JSON/jsoncpp.cpp config/*.cpp
#OBJECTS 	= $(SOURCES:.cpp=.o)

.PHONY:		main

%.o:	%.cpp
	$(CC) -c $(CC_FLAGS) $(INC_PARAMS) $< -o $@

%.o:	%.c
	$(C_C) -c $(CC_FLAGS) $(INC_PARAMS) $< -o $@

all:		clearing clean server

clearing:
	clear

clean:
	rm -f src/*.o frontend/*.o JSON/*.o

server:		src/main.o src/locator.o src/parser.o src/frontend/frontend.o src/frontend/MicroHTTPFrontend.o src/JSON/jsoncpp.o
	$(CC) $(CFLAGS) $(LIB_PARAMS) -o mgserv2 $^ libs/libmicrohttpd-0.9.73/src/microhttpd/.libs/libmicrohttpd.a -lpthread
