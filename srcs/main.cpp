#include "server.hpp"

void	drain_socket(int sockfd)
{
	char tmp[1024];
	ssize_t extra;

	do
	{
		extra = recv(sockfd, tmp, sizeof(tmp), MSG_DONTWAIT);
	} while (extra > 0);
	
}

void	c_server::create_socket()
{
	this->_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_socket_fd < 0){
		cerr << "Error: socket creation - " << errno << endl;
		return ;
	}

	int socket_option = 1;
	if (setsockopt(this->_socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) < 0) {
		cerr << "Error: socket option - Reusable address - " << errno << endl;
		close(this->_socket_fd);
		return ;
	}
}

void c_server::bind_and_listen()
{
	this->_socket_address = sockaddr_in();
	this->_socket_address.sin_family = AF_INET;
	this->_socket_address.sin_port = htons(8080);
	this->_socket_address.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(this->_socket_fd, (struct sockaddr *) &this->_socket_address, sizeof(this->_socket_address)) < 0)
	{
		cerr << "Error: Bind mode - " << errno << endl;
		return ;
	}
	
	if (listen(this->_socket_fd, SOMAXCONN) < 0){
		cerr << "Error: Listening mode - " << errno << endl;
		return ;
	}
}

/* Appliquer un fd en mode non bloquant les fonctions de base ne bloque plus le chemin si l'operation ne peux pas etre faite tout de suite
ca echoue directement et permet de lancer correctement la boucle de poll()
* On check les flags du fd socket: rdonly, wronly, rdwr, append, sync, nonblock
* FCNTL = file controle - > fonction générique pour manipuler des fd
* Si un fd est ouvert en écriture/lecture avec append, alors flags aura O_RDWR et O_APPEND
*/

void c_server::set_non_blocking(int fd)
{
	/* F_GETFL: récupère les file status (les options/flags du fd) */
	int flags = fcntl(fd, F_GETFL, 0); // 
	if (flags < 0)
	{
		cerr << "Error: F_GETFL - " << errno << endl;
		return ;
	}

	/* F_SETFL: permet de changer les flags -> ici on ajoute NONBLOCK */
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		cerr << "Error: F_SETFL - " << errno << endl;
		return ;
	}
}

int main(void)
{
	c_server server;
	
	server.create_socket();
	server.bind_and_listen();
	//server.set_non_blocking(server.get_socket_fd());
	
	struct sockaddr_in socket_address = server.get_socket_addr();
	int socket_fd = server.get_socket_fd();
	int connected_socket_fd;
	c_response response_handler;

	while (true)
	{
		cout << "Waiting for connections..." << endl;

		socklen_t addrlen = sizeof(socket_address);
		connected_socket_fd = accept(socket_fd, (struct sockaddr *) &socket_address, &addrlen);
		
	
		if (connected_socket_fd < 0)
		{
			cerr << "Error: Accepting mode - " << errno << endl;
			return (-1);
		}
		char ip_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(socket_address.sin_addr), ip_str, INET_ADDRSTRLEN);
		c_request my_request(ip_str);
		
		cout << "New client connected !\n" << endl;

		bool	keep_alive = true;
		while (keep_alive)
        {
			my_request.read_request(connected_socket_fd);
			if (my_request.get_error())
			{
				keep_alive = false;
				break;
			}
			drain_socket(connected_socket_fd);
			if (!keep_alive)
				break;

			my_request.print_full_request();

			response_handler.define_response_content(my_request);
			const string &response = response_handler.get_response();
			if (send(connected_socket_fd, response.data(), response.size(), 0) == -1)
			{
				cerr << "Error: Message not sent - " << errno << endl;
				keep_alive = false;
				break;
			}
			if (!keep_alive || my_request.get_header_value("Connection") == "close")
					break;
		}
		close(connected_socket_fd);		
	}
	close(socket_fd);
	return (0);
}
