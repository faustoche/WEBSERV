#include "server.hpp"

void c_server::create_socket_for_each_port(const std::vector<int>&ports)
{
	for (std::vector<int>::const_iterator it = ports.begin(); it != ports.end(); it++)
	{
		int port = *it;
		int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_fd < 0)
		{
			cerr << "Error: Socket's creation for port " << port << " - " << errno << endl;
			continue ;
		}
		int socket_option = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) < 0)
		{
			cerr << "Error: Socket's option for port " << port << " - " << errno << endl;
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
			cerr << "Error: bind for port " << port << " - " << errno << endl;
			close (socket_fd);
			continue ;
		}
		if (listen(socket_fd, SOMAXCONN) < 0)
		{
			cerr << "Error: listen for port " << port << " - " << errno << endl;
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

void	c_server::close_all_sockets_and_fd(void)
{
	for (map<int, c_client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		int client_fd = it->first;
		close(client_fd);
	}
	_clients.clear();
	cout << GREEN << "🐕 Clients connexions closed (" << _clients.size() << ")" << RESET << endl;

	for (std::map<int, c_cgi*>::iterator it = _active_cgi.begin(); it != _active_cgi.end(); )
	{
	    std::map<int, c_cgi*>::iterator to_delete = it++;
	    if (to_delete->second)
		{
			pid_t pid = to_delete->second->get_pid();
            kill(pid, SIGTERM);
            waitpid(pid, NULL, 0);
	        delete to_delete->second;
		}
	    _active_cgi.erase(to_delete);
	}
	cout << GREEN << "🧬 Active CGI has been cleaned !" << RESET << endl;

	for (map<int, int>::iterator it = _multiple_ports.begin(); it != _multiple_ports.end(); it++)
	{
		int socket_fd = it->first;
		close(socket_fd);
	}
	_multiple_ports.clear();
	cout << GREEN << "🧦 Sockets closed!" << RESET << endl;
	_poll_fds.clear();
	cout << GREEN << "✅ SERVER CLOSED!" << RESET << endl;
}