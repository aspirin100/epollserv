#include <iostream>
#include <memory>
#include <string>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

int main()
{
	std::unique_ptr<sockaddr_in> saddr{new sockaddr_in};

	auto sock_closer = [](int *sock_ptr)
	{
		if (sock_ptr)
			close(*sock_ptr);

		delete sock_ptr;
	};

	std::unique_ptr<int, decltype(sock_closer)> sock(new int, sock_closer);

	int user_choice;
	std::cout << "choose connection type: 1 - TCP, 2 - UDP; DEFAULT:TCP\n";
	std::cin >> user_choice;

	int socket_type = SOCK_STREAM;

	if (user_choice == 2)
		socket_type = SOCK_DGRAM;

	if (*sock = socket(AF_INET, socket_type, 0); *sock < 0)
	{
		perror("socket");
		return -1;
	}

	short port;
	std::string host;

	std::cout << "enter server address\n";
	std::cin >> host;

	std::cout << "enter server port:\n";
	std::cin >> port;

	saddr->sin_family = AF_INET;
	saddr->sin_port = htons(port);

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = socket_type;

	if (int status = getaddrinfo(host.c_str(), NULL, &hints, &res); status != 0)
	{
		std::cout << "getaddrinfo fail: " << gai_strerror(status) << std::endl;

		hostent *he = gethostbyname(host.c_str());
		if (he == nullptr)
		{
			std::cerr << "gethostbyname fail: " << hstrerror(h_errno) << std::endl;
			return -1;
		}

		memcpy(&saddr->sin_addr, he->h_addr_list[0], he->h_length);
	}
	else
	{
		sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
		memcpy(&saddr->sin_addr, &addr->sin_addr, sizeof(addr->sin_addr));
		freeaddrinfo(res);
	}

	if (connect(*sock, reinterpret_cast<sockaddr *>(saddr.get()), sizeof(*saddr)) < 0)
	{
		perror("connect");
		return -1;
	}

	std::cout << "connected\n";

	std::string msg, received_msg;
	while (true)
	{
		std::cin >> msg;
		received_msg.resize(128);

		msg.push_back('\n'); // end of msg sign for server

		if (send(*sock, msg.data(), msg.size(), 0) < 0)
		{
			perror("send");
			continue;
		}

		int received = recv(*sock, received_msg.data(), received_msg.size(), 0);
		std::cout << "server: " << received_msg.substr(0, received) << "\n";
	}

	return 0;
}
