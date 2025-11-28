.PHONY: all clean

NAME := client

CXX := g++
CXX_STANDARD := c++17

all: $(NAME)

$(NAME): client.o
	$(CXX) client.o -o $(NAME)

client.o:
	$(CXX) -std=$(CXX_STANDARD) -c client.cpp -o client.o

clean:
	rm *.o
	rm client