CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread
LDFLAGS = -pthread

# Detect OS
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    RM = del /Q
    TARGET_EXT = .exe
else
    RM = rm -f
    TARGET_EXT =
endif

# Targets
TRACKER_TARGET = tracker$(TARGET_EXT)
PEER_TARGET = peer$(TARGET_EXT)

# Source files
COMMON_SRCS = common.cpp network.cpp fileutils.cpp
TRACKER_SRCS = tracker.cpp $(COMMON_SRCS)
PEER_SRCS = peer.cpp $(COMMON_SRCS)

# Object files
COMMON_OBJS = $(COMMON_SRCS:.cpp=.o)
TRACKER_OBJS = $(TRACKER_SRCS:.cpp=.o)
PEER_OBJS = $(PEER_SRCS:.cpp=.o)

# Default target
all: $(TRACKER_TARGET) $(PEER_TARGET)

# Tracker
$(TRACKER_TARGET): $(TRACKER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Peer
$(PEER_TARGET): $(PEER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean #edited by manas
clean:
	$(RM) *.o $(TRACKER_TARGET) $(PEER_TARGET)

# Dependencies
common.o: common.cpp common.h
network.o: network.cpp network.h common.h
fileutils.o: fileutils.cpp fileutils.h common.h network.h
tracker.o: tracker.cpp tracker.h common.h network.h
peer.o: peer.cpp peer.h common.h network.h fileutils.h

.PHONY: all clean
