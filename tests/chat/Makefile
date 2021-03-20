PROJECT := $(shell pwd)
SRCDIR  := $(PROJECT)/../../
# TEST := $(wildcard $(PROJECT)/codec.cpp)
SRC := $(wildcard $(SRCDIR)/src/*.cpp)
# override SRC := $(SRC) $(TEST) 
CXX := g++
INCLUDE := -I $(SRCDIR)/src/ -I $(PROJECT)
CFLAGS := -std=c++11 -g -pg -Wall -O3 -D_PTHREADS
LIBS := -lpthread


TARGET1 := chat_server
# TARGET2 := pub

$(TARGET1): $(SRC) server.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

# $(TARGET2): $(SRC) pubsub.cpp pub.cpp
# 	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET1) | xargs rm -f
	# find . -name $(TARGET2) | xargs rm -f