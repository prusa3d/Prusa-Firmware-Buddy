#
# This is GNU makefile for Linux OS and GCC/Clang compiler
#

# C++ compiler
CXX = g++

# C++ compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -Wfatal-errors -I ../include -DNDEBUG -O3

# Command for removing all files and directories
RM_RF = rm -rf

# List of all source files that must be compiled and tested
SRCS := $(wildcard *.cpp)

# First rule
.PHONY: all
all:

# This function generates build rule for given source file.
# Usage: $(call generate_rules,source_file)
define generate_rules

.PHONY: $(basename $(1))
all:    $(basename $(1))
$(basename $(1)):
	$(CXX) $(CXXFLAGS) $(1) -o $(basename $(1)).out

endef

# Generate rules for all source files
$(foreach file,$(SRCS),$(eval $(call generate_rules,$(file))))

.PHONY: clean
clean:
	$(RM_RF) *.out
