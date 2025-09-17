#include "server.hpp"

void c_server::create_socket_for_each_port(const std::vector<int>&ports)
{
	for (std::vector<int>::const_iterator it = ports.begin(); it != ports.end(); it++)
	{
		int port = *it;
		int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_fd < 0)
		{
			cerr << "Error: Création de la socket pour le port " << port << " - " << errno << endl;
			continue ;
		}
		int socket_option = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) < 0)
		{
			cerr << "Error: Création de l'option pour le port " << port << " - " << errno << endl;
			close(socket_fd);
			continue ;
		}
		sockaddr_in socket_address;
		memset(&socket_address, 0, sizeof(socket_address));
		socket_address.sin_family = AF_INET;
		socket_address.sin_port = htons(port);
		socket_address.sin_addr.s_addr = INADDR_ANY;

		if (bind(socket_fd, (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0)
		{
			cerr << "Error: bind pour le port " << port << " - " << errno << endl;
			close (socket_fd);
			continue ;
		}
		if (listen(socket_fd, SOMAXCONN) < 0)
		{
			cerr << "Erro: listen pour le port " << port << " - " << errno << endl;
			close(socket_fd);
			continue ;
		}
		set_non_blocking(socket_fd);
		_multiple_ports[socket_fd] = port;
	}
}

bool c_server::is_listening_socket(int fd)
{
	return (_multiple_ports.find(fd) != _multiple_ports.end());
}

int c_server::get_port_from_socket(int socket_fd)
{
	std::map<int, int>::iterator it = _multiple_ports.find(socket_fd);
	if (it != _multiple_ports.end())
		return (it->second);
	return (-1);
}