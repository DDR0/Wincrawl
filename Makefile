# Main Makefile, intended for use on Linux/X11 and compatible platforms
# using GNU Make.
#
# The main options are:
#
#   CCACHE           The ccache binary that should be used when USE_CCACHE is
#                     enabled (see below). Defaults to 'ccache'.
#   CXX              C++ compiler comand line.
#   CXXFLAGS         Additional C++ compiler options.
#   OPTIMIZE         If set to 'yes' (default), builds with compiler
#                     optimizations enabled (-O2). You may alternatively use
#                     CXXFLAGS to set your own optimization options.
#   LDFLAGS          Additional linker options.
#   USE_CCACHE       If set to 'yes' (default), builds using the CCACHE binary
#                     to run the compiler. If ccache is not installed (i.e.
#                     found in PATH), this option has no effect.

OPTIMIZE?=yes
USE_LUA?=yes
USE_BOX2D?=yes

CCACHE?=ccache
USE_CCACHE?=$(shell which $(CCACHE) > /dev/null 2>&1 && echo yes)
ifneq ($(USE_CCACHE),yes)
CCACHE=
USE_CCACHE=no
endif

SANITIZE_ADDRESS?=
ifneq ($(SANITIZE_ADDRESS), yes)
SANITIZE_ADDRESS=no
endif

SANITIZE_UNDEFINED?=
ifneq ($(SANITIZE_UNDEFINED), yes)
SANITIZE_UNDEFINED=no
endif

ifeq ($(OPTIMIZE),yes)
BASE_CXXFLAGS += -O2
endif

BASE_CXXFLAGS += -Wall -Werror

# Initial compiler options, used before CXXFLAGS and CPPFLAGS. -rdynamic -Wno-literal-suffix
BASE_CXXFLAGS += -std=c++20 -Wall \
	-g -fno-inline-functions -lpthread \
	-fthreadsafe-statics \
	-Wno-narrowing -Wno-reorder -Wno-unused \
	-Wno-unknown-pragmas -Wno-overloaded-virtual \
	-fstrict-enums

LDFLAGS?=-rdynamic

# Check for sanitize-address option
ifeq ($(SANITIZE_ADDRESS), yes)
BASE_CXXFLAGS += -g3 -fsanitize=address
LDFLAGS += -fsanitize=address
endif

# Check for sanitize-undefined option
ifeq ($(SANITIZE_UNDEFINED), yes)
BASE_CXXFLAGS += -fsanitize=undefined
endif

BUILD_DIR := ./build

SRC       := $(wildcard ./Wincrawl2/*.cpp)
OBJ       := $(patsubst ./Wincrawl2/%.cpp,./build/%.o,$(SRC))
DEPS      := $(patsubst ./Wincrawl2/%.cpp,./build/%.d,$(SRC))
INCLUDES  := $(addprefix -I,./Wincrawl2)

vpath %.cpp Wincrawl2

CPPFLAGS += -MMD -MP
define cc-command
$1/%.o: %.cpp
	@echo "Building:" $$<
	@$(CCACHE) $(CXX) $(CPPFLAGS) $(BASE_CXXFLAGS) $(CXXFLAGS) $(INCLUDES) -MF $$@.d -c -o $$@ $$<
endef

.PHONY: all checkdirs clean

all: checkdirs wincrawl

wincrawl: $(OBJ)
	@echo "Linking : wincrawl"
	@#I don't know why, but -lpthread has to come after the .o files or it doesn't work.
	@$(CXX) \
		$(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) \
		$(OBJ) -lpthread -o wincrawl

checkdirs: $(BUILD_DIR)
	@printf "\
	OPTIMIZE            : $(OPTIMIZE)\n\
USE_CCACHE          : $(USE_CCACHE)\n\
	CCACHE              : $(CCACHE)\n\
SANITIZE_ADDRESS    : $(SANITIZE_ADDRESS)\n\
SANITIZE_UNDEFINED  : $(SANITIZE_UNDEFINED)\n\
CXX                 : $(CXX)\n\
	BASE_CXXFLAGS       : $(BASE_CXXFLAGS)\n\
	CXXFLAGS            : $(CXXFLAGS)\n\
LDFLAGS             : $(LDFLAGS)\n"


$(BUILD_DIR):
	@mkdir -p $@

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.o.d wincrawl

$(eval $(call cc-command,$(BUILD_DIR)))

# pull in dependency info for *existing* .o files
-include $(OBJ:.o=.o.d)

debug:
	make && gdb -ex run -tui wincrawl