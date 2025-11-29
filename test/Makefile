NAME := client
.PHONY: all clean $(NAME)


CXX := g++
CXX_STANDARD := c++17
CXXFLAGS := -std=$(CXX_STANDARD)

all: $(NAME)

$(NAME): client.o
	$(CXX) client.o -o $(NAME)

client.o:
	$(CXX) $(CXXFLAGS) -c client.cpp -o client.o

clean:
	rm *.o
	rm client

build-img:
	docker build . -t $(NAME)_img

rm-img:
	-docker image rm $(NAME)_img 2>/dev/null

NETWORK := server-net

interact:
	-docker network create $(NETWORK) 2>/dev/null
	docker run --rm -it \
	--name $(NAME) \
	--dns=8.8.8.8 --dns=1.1.1.1 \
	--network $(NETWORK) \
	 $(NAME)_img 