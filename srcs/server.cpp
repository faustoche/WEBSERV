#include "../includes/server.hpp"

int main(void)
{
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in socket_address = sockaddr_in();

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(8080);
	socket_address.sin_addr.s_addr = INADDR_ANY;


	if (socket_fd < 0){
		cerr << "Error: Socket creation - " << errno << endl;
		return (-1);
	}

	if (bind(socket_fd, (struct sockaddr *) &socket_address, sizeof(socket_address)) < 0){
		cerr << "Error: Bind mode - " << errno << endl;
		return (-1);
	}

	if (listen(socket_fd, SOMAXCONN) < 0){
		cerr << "Error: Listening mode - " << errno << endl;
		return (-1);
	}

	int connected_socket_fd;

	while (true)
	{
		socklen_t addrlen = sizeof(socket_address);
		connected_socket_fd = accept(socket_fd, (struct sockaddr *) &socket_address, &addrlen);
	
		if (connected_socket_fd < 0){
			cerr << "Error: Accepting mode - " << errno << endl;
			return (-1);
		}

		char answer[50] = "HTTP/1.1 200 OK \r\n\r\n Vive Snoopy!\n";
		if (send(connected_socket_fd, answer, strlen(answer), 0) == -1){
			cerr << "Error: Message not sent - " << errno << endl;
			return (-1);
		}
	}
	close(socket_fd);
	close(connected_socket_fd);
	return (0);
}