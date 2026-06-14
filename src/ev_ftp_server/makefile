CXX ?= g++

TARGET := ev_ftp_server

SOURCES := \
	ev_ftp_server.cpp \
	XFtpFactory.cpp \
	XFtpLIST.cpp \
	XFtpPORT.cpp \
	XFtpRETR.cpp \
	XFtpServerCMD.cpp \
	XFtpSTOR.cpp \
	XFtpTask.cpp \
	XFtpUSER.cpp \
	XThread.cpp \
	XThreadPool.cpp

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
