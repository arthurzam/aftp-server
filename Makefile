CXX=g++
LIBS=-lssl -lcrypto
CXXFLAGS+=-std=c++0x -c -Wall -Wextra -flto -fno-exceptions -fno-rtti
LDFLAGS+=-flto -fno-exceptions -fno-rtti
SOURCES=IOThreadPool.cpp fileControl.cpp FileTransfer.cpp LoginDB.cpp Login.cpp User.cpp UserList.cpp server.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=aftp-server
REMOVEFILECOMMAND=rm -f

ifeq ($(OS),Windows_NT)
	LIBS+=-lws2_32
	CXXFLAGS+=-DWIN32
else
	LIBS+=-lpthread
endif

all: $(SOURCES) $(EXECUTABLE)

debug: CXXFLAGS += -DDEBUG -g
debug: LDFLAGS += -DDEBUG -g
debug: $(SOURCES) $(EXECUTABLE)
	
	
$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	$(REMOVEFILECOMMAND) *.o $(EXECUTABLE)*

install:
	install -Dm 755 $(EXECUTABLE) "$(DESTDIR)/usr/bin/$(EXECUTABLE)"

uninstall:
	rm "$(DESTDIR)/usr/bin/$(EXECUTABLE)"
