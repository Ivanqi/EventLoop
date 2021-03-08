PROJECT := $(shell pwd)
SRCDIR  := $(PROJECT)/../../
TEST := $(wildcard $(PROJECT)/*.cpp)
SRC := $(wildcard $(SRCDIR)/src/*.cpp)
# override SRC := $(SRC) $(TEST) 
CXX := g++
INCLUDE := -I $(SRCDIR)/src/ -I $(PROJECT)
CFLAGS := -std=c++11 -g -pg -Wall -O3 -D_PTHREADS
LIBS := -lpthread


TARGET1 := idleconnection
TARGET2 := sortedlist


$(TARGET1): $(SRC) echo.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

$(TARGET2): $(SRC) sortedlist.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f