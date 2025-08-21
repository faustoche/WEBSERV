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

	while (true)
	{
		cout << "Waiting for connections..." << endl;

		socklen_t addrlen = sizeof(socket_address);
		connected_socket_fd = accept(socket_fd, (struct sockaddr *) &socket_address, &addrlen);
	
		if (connected_socket_fd < 0){
			cerr << "Error: Accepting mode - " << errno << endl;
			return (-1);
		}
		cout << "New client connected !" << endl;

		while (true) // while (keep_alive)
        {
            /* Recevoir un message */
        	string request;
            char        buffer[BUFFER_SIZE];
            int         receivedBytes;
            
            /* Lire jusqu'a la fin des headers (\r\n\r\n)*/
            while (request.find("\r\n\r\n") ==string::npos)
            {
            	receivedBytes = recv(connected_socket_fd, buffer, sizeof(buffer) - 1, 0);
                if (receivedBytes <= 0)
                {
                   cerr << " Error: message not received - " << errno << endl;
                    break ;
                }
                buffer[receivedBytes] = '\0';
                request.append(buffer);
               	fill(buffer, buffer + sizeof(buffer), '\0');
            }
           cout << "\nRequete client:\n" << request << endl;

            /*if (!keep_alive)
               break ;

            if (request.find("Connection: close") != string::npos)
            {
                keep_alive = false;
            }*/

			char answer[50] = "HTTP/1.1 200 OK \r\n\r\n Vive Snoopy!\n";
			if (send(connected_socket_fd, answer, strlen(answer), 0) == -1)
			{
				cerr << "Error: Message not sent - " << errno << endl;
				return (-1);
			}
		}
		close(connected_socket_fd);		
	}
	close(socket_fd);
	return (0);
}