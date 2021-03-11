PROJECT := $(shell pwd)
SRCDIR  := $(PROJECT)/../../
TEST := $(wildcard $(PROJECT)/*.cpp)
SRC := $(wildcard $(SRCDIR)/src/*.cpp)
# override SRC := $(SRC) $(TEST) 
CXX := g++
INCLUDE := -I $(SRCDIR)/src/ -I $(PROJECT)
CFLAGS := -std=c++11 -g -pg -Wall -O3 -D_PTHREADS
LIBS := -lpthread


TARGET1 := TcpClient1_test
TARGET2 := TcpClient2_test
TARGET3 := TcpClient3_test

$(TARGET1): $(SRC) TcpClient_1.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

$(TARGET2): $(SRC) TcpClient_2.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

$(TARGET3): $(SRC) TcpClient_3.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f