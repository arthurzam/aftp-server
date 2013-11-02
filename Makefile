CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=fileControl.cpp md5.cpp server.cpp User.cpp UserList.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=aftp_server
REMOVEFILECOMMAND=

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lws2_32
	EXECUTABLE += .exe
	REMOVEFILECOMMAND = del
else
	LDFLAGS += -lpthread
	REMOVEFILECOMMAND = rm -f
endif

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clear: clean

clean:
	$(REMOVEFILECOMMAND) *.o $(EXECUTABLE)
