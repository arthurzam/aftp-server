CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=fileControl.cpp FileTransfer.cpp LoginDB.cpp Login.cpp User.cpp UserList.cpp server.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=aftp_server
LIBS=
OPENSSL_LIB=-lssl -lcrypto
REMOVEFILECOMMAND=rm -f

ifeq ($(OS),Windows_NT)
	LIBS = -lws2_32
else
	LIBS = -lpthread
endif

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS) $(OPENSSL_LIB)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(REMOVEFILECOMMAND) *.o $(EXECUTABLE)*
