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

int main(void)
{
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in socket_address = sockaddr_in();

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(8080);
	socket_address.sin_addr.s_addr = INADDR_ANY; // adresse IP du .conf

	if (socket_fd < 0){
		cerr << "Error: Socket creation - " << errno << endl;
		return (-1);
	}

	int socket_option = 1;

	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) < 0) {
		cerr << "Error: socket option - Reusable address - " << errno << endl;
		close(socket_fd);
		return (-1);
	}

	if (bind(socket_fd, (struct sockaddr *) &socket_address, sizeof(socket_address)) < 0)
	{
		cerr << "Error: Bind mode - " << errno << endl;
		return (-1);
	}

	if (listen(socket_fd, SOMAXCONN) < 0){
		cerr << "Error: Listening mode - " << errno << endl;
		return (-1);
	}

	int connected_socket_fd;
	c_response response_handler;

	while (true)
	{
		cout << "Waiting for connections..." << endl;

		socklen_t addrlen = sizeof(socket_address);
		connected_socket_fd = accept(socket_fd, (struct sockaddr *) &socket_address, &addrlen);
		c_request my_request;
	
		if (connected_socket_fd < 0)
		{
			cerr << "Error: Accepting mode - " << errno << endl;
			return (-1);
		}
		cout << "New client connected !\n" << endl;

		bool	keep_alive = true;
		while (keep_alive)
        {
			int status_code;
			status_code = my_request.read_request(connected_socket_fd);
			if (status_code == 400 || status_code == 408 || status_code == 413)
			{
				my_request.set_status_code(status_code);
				close(connected_socket_fd);
				keep_alive = false;
			}
			drain_socket(connected_socket_fd);

			my_request.print_full_request();

			if (!keep_alive)
				break;

			// response_handler.define_response_content(my_request);

			// const string &response = response_handler.get_response();
			// if (send(connected_socket_fd, response.data(), response.size(), 0) == -1)
			// {
			// 	cerr << "Error: Message not sent - " << errno << endl;
			// 	keep_alive = false;
			// 	break;
			// }
			if (!keep_alive)
				break;
		}
		close(connected_socket_fd);		
	}
	close(socket_fd);
	return (0);
}