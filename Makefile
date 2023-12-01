#Compiler
CXX              := g++

TARGET           := fast-chess

# Source, Includes
SRCDIR           := src
TESTDIR          := tests
BUILDDIR         := tmp
INCDIR           := src

#Flags, Libraries and Includes
CXXFLAGS         := -O3 -std=c++17 -Wall -Wextra -DNDEBUG
CXXFLAGS_TEST	 := -O2 -std=c++17 -Wall -Wextra -pedantic -Wuninitialized -g3 -fno-omit-frame-pointer
INC              := -I$(INCDIR) -Ithird_party

SRC_FILES        := $(shell find $(SRCDIR) -name "*.cpp")
SRC_FILES_TEST   := $(shell find $(TESTDIR) -maxdepth 1 -name "*.cpp")

# Windows file extension
SUFFIX           := .exe

NATIVE 	         := -march=native

ifeq ($(MAKECMDGOALS),$(TESTDIR))
	CXXFLAGS  := $(CXXFLAGS_TEST)
	SRC_FILES := $(filter-out src/main.cpp, $(SRC_FILES)) $(SRC_FILES_TEST)
	TARGET    := $(TARGET)-tests
	NATIVE    := 
endif

OBJECTS   := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(SRC_FILES))
DEPENDS   := $(patsubst %.cpp,$(BUILDDIR)/%.d,$(SRC_FILES))
DEPFLAGS  := -MMD -MP
MKDIR	  := mkdir -p

ifeq ($(OS), Windows_NT)
	uname_S  := Windows
	LDFLAGS  := -static -static-libgcc -static-libstdc++ -Wl,--no-as-needed
else
ifeq ($(COMP), MINGW)
	uname_S  := Windows
else
	SUFFIX  :=
	LDFLAGS := -pthread
	uname_S := $(shell uname -s)
endif
endif

ifeq ($(build), debug)
	CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -pedantic -Wuninitialized -g3
endif

ifeq ($(build), release)
	LDFLAGS  := -lpthread -static -static-libgcc -static-libstdc++ -Wl,--no-as-needed
	NATIVE   := -march=x86-64
endif

# Different native flag for macOS
ifeq ($(uname_S), Darwin)
	NATIVE =
	LDFLAGS =
endif

# Compile with address sanitizer
ifeq ($(san), asan)
	LDFLAGS += -fsanitize=address
endif

# Compile with memory sanitizer
ifeq ($(san), memory)
	LDFLAGS += -fsanitize=memory -fPIE -pie
endif

# Compile with undefined behavior sanitizer
ifeq ($(san), undefined)
	LDFLAGS += -fsanitize=undefined
endif

# Compile with thread sanitizer
ifeq ($(san), thread)
	LDFLAGS += -fsanitize=thread
endif

# Versioning
GIT_SHA = $(shell git rev-parse --short HEAD 2>/dev/null)
ifneq ($(GIT_SHA), )
	CXXFLAGS += -DGIT_SHA=\"$(GIT_SHA)\"
endif

GIT_DATE = $(shell git show -s --date=format:'%Y%m%d' --format=%cd HEAD 2>/dev/null)
ifneq ($(GIT_DATE), )
	CXXFLAGS += -DGIT_DATE=\"$(GIT_DATE)\"
endif

# Compile with Cutechess output support
ifeq ($(USE_CUTE), true)
	CXXFLAGS += -DUSE_CUTE
endif

.PHONY: clean all tests FORCE

all: $(TARGET)

tests: $(TARGET)
	$(CXX) $(CXXFLAGS) ./tests/mock/engine/dummy_engine.cpp -o ./tests/mock/engine/dummy_engine$(SUFFIX) $(LDFLAGS)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(NATIVE) $(INC) $(DEPFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp | build_directories
	$(CXX) $(CXXFLAGS) $(NATIVE) $(INC) $(DEPFLAGS) -c $< -o $@ $(LDFLAGS)

build_directories:
	@$(MKDIR) $(BUILDDIR)
	@find src -type d -exec $(MKDIR) $(BUILDDIR)/{} \;
	@find tests -type d -exec $(MKDIR) $(BUILDDIR)/{} \;

clean:
	rm -rf $(BUILDDIR) $(TARGET) *.exe

-include $(DEPENDS)

# Force rebuild of options.o for accurate versioning
tmp/src/options.o: FORCE
FORCE: