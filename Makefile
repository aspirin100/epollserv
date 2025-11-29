PROJECT := epollserv
.PHONY: all clean $(PROJECT)

PROJECT_DIR := $(shell /bin/pwd)
INCDIR := $(PROJECT_DIR)/include
SRCDIR := $(PROJECT_DIR)/src
OBJ_DIR := $(PROJECT_DIR)/objects

CXX := g++
CXX_STANDARD := c++17
CXXFLAGS := -I$(INCDIR) -std=$(CXX_STANDARD)

CPPS := $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPPS))

all: $(PROJECT)

$(PROJECT): $(OBJ)
	$(CXX) $(OBJ) -o $(PROJECT)

$(OBJ_DIR)/%.o : $(SRCDIR)/%.cpp
	@mkdir -p $(PROJECT_DIR)/objects
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm $(PROJECT)


build-img:
	docker build . -t $(PROJECT)_img

NETWORK := server-net

start:
	-docker network create $(NETWORK) 2>/dev/null
	docker run --rm -d \
		--name $(PROJECT) \
		--network $(NETWORK) \
		 -p 8888:8888 $(PROJECT)_img

stop:
	docker stop $(PROJECT)