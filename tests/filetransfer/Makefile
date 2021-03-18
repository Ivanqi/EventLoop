PROJECT := $(shell pwd)
SRCDIR  := $(PROJECT)/../../
TEST := $(wildcard $(PROJECT)/*.cpp)
SRC := $(wildcard $(SRCDIR)/src/*.cpp)
# override SRC := $(SRC) $(TEST) 
CXX := g++
INCLUDE := -I $(SRCDIR)/src/ -I $(PROJECT)
CFLAGS := -std=c++11 -g -pg -Wall -O3 -D_PTHREADS
LIBS := -lpthread


TARGET1 := download
TARGET2 := download2
TARGET3 := download3


$(TARGET1): $(SRC) download.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

$(TARGET2): $(SRC) download2.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

$(TARGET3): $(SRC) download3.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f