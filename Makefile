CC=g++
SOURCES=fileControl.cpp FileTransfer.cpp LoginDB.cpp Login.cpp User.cpp UserList.cpp server.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=aftp_server
LIBS=-lssl -lcrypto
REMOVEFILECOMMAND=rm -f

ifeq ($(OS),Windows_NT)
	LIBS+=-lws2_32
else
	LIBS+=-lpthread
endif

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) -std=c++0x $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) -std=c++0x -c -Wall $(CFLAGS) $< -o $@

clean:
	$(REMOVEFILECOMMAND) *.o $(EXECUTABLE)*
