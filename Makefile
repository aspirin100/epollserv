.PHONY: all clean

PROJECT := epollserv

# directories
PROJECT_DIR := $(shell /bin/pwd)
INCDIR := $(PROJECT_DIR)/include
SRCDIR := $(PROJECT_DIR)/src
OBJ_DIR := $(PROJECT_DIR)/objects

# cxx 
CXX := g++
CXX_STANDARD := c++17

FLAGS := -I$(INCDIR) -std=$(CXX_STANDARD)

# objects accumulation
CPPS := $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPPS))

# instructions
all: $(PROJECT)

$(PROJECT): $(OBJ)
	$(CXX) $(OBJ) -o $(PROJECT)

$(OBJ_DIR)/%.o : $(SRCDIR)/%.cpp
	@mkdir -p $(PROJECT_DIR)/objects
	$(CXX) $(FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm $(PROJECT)
