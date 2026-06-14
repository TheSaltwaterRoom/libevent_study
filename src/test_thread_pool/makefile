CXX ?= g++

TARGET := test_thread_pool

SOURCES := \
	test_thread_pool.cpp \
	XThreadPool.cpp \
	XThread.cpp \
	XFtpServerCMD.cpp

OBJECTS := $(SOURCES:.cpp=.o)
DEPFILES := $(OBJECTS:.o=.d)

CPPFLAGS ?=
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?= -levent -pthread

.DEFAULT_GOAL := all

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPFILES)

-include $(DEPFILES)

.PHONY: all run clean
