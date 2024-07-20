CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g -pthread

TARGET = server

OBJS = ../code/Buffer/*.cpp ../code/SQL/*.cpp ../code/Log/*.cpp ../code/ThreadPool/*.cpp \
	   ../code/HeapTimer/*.cpp ../code/HttpRequest/*.cpp ../code/HttpResponse/*.cpp \
	   ../code/HttpConn/*.cpp ../code/Epoller/*.cpp ../code/Server/*.cpp  ../code/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)