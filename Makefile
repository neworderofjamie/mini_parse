# Determine whether OS is Darwin i.e. Mac OS X and whether the kernel is 32 or 64 bit
OS_SIZE				:=$(shell getconf LONG_BIT)
OS_UPPER			:=$(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
OS_RELEASE_UPPER		:=$(shell uname -r 2>/dev/null | tr [:lower:] [:upper:])
DARWIN				:=$(strip $(findstring DARWIN,$(OS_UPPER)))
MICROSOFT			:=$(strip $(findstring MICROSOFT,$(OS_RELEASE_UPPER)))

# Get directory of this makefile i.e. mini-parse directory (means make can be invoked else where)
MINI_PARSE_DIR			:= $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# Set standard compiler and archiver flags
CXXFLAGS			+=-Wall -Wpedantic -Wextra -MMD -MP -I$(MINI_PARSE_DIR)/include
ARFLAGS				:=-rcs

ifdef DEBUG
    MINI_PARSE_PREFIX		:=$(MINI_PARSE_PREFIX)_debug
    CXXFLAGS			+=-g -O0 -DDEBUG
endif 

MINI_PARSE			:=$(MINI_PARSE_DIR)/mini_parse$(GENN_PREFIX)

# Find source files
SOURCES				:=$(wildcard src/*.cc)

# Add prefix to object directory and library name
OBJECT_DIRECTORY		?=$(MINI_PARSE_DIR)/obj$(MINI_PARSE_PREFIX)

# Add object directory prefix
OBJECTS			:=$(SOURCES:%.cc=$(OBJECT_DIRECTORY)/%.o)
DEPS			:=$(OBJECTS:.o=.d)

# Default to C++17 but allow this to overriden
CXX_STANDARD		?=c++17

.PHONY: all clean

all: $(MINI_PARSE)

$(MINI_PARSE): $(OBJECTS)
	mkdir -p $(@D)
	$(CXX) -std=$(CXX_STANDARD) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

-include $(DEPS)

$(OBJECT_DIRECTORY)/%.o: %.cc $(OBJECT_DIRECTORY)/%.d
	mkdir -p $(@D)
	$(CXX) -std=$(CXX_STANDARD) $(CXXFLAGS) -c -o $@ $<

%.d: ;

clean:
	@find $(OBJECT_DIRECTORY) -type f -name "*.o" -delete
	@find $(OBJECT_DIRECTORY) -type f -name "*.d" -delete
	@rm -f $(MINI_PARSE)
