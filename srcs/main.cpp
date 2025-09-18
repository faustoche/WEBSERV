#include "server.hpp"
#include "werbserv_config.hpp"

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


void	run_multiserver(vector<c_server> servers)
{
	while (true)
	{
		for (size_t i = 0; i < servers.size(); ++i)
		{
			servers[i].setup_pollfd();
			servers[i].handle_poll_events();
		}
	}
}

int main(int argc, char **argv) //main du parsing
{
    try
    {
        if (argc != 2)
			throw invalid_argument("The program must have one argument (the configuration file)");

		c_webserv_config webserv(argv[1]);

		vector<c_server> servers = webserv.get_servers();
		if (servers.empty())
			throw invalid_argument("Error: No servers configurations");

		servers.resize(1); //limite vecteur a 1 seul c_server
		
		webserv.print_configurations();

		// initialisation des serveurs
		for (size_t i = 0; i < servers.size(); ++i)
		{
			servers[i].create_socket();
			servers[i].bind_and_listen();
			servers[i].set_non_blocking(servers[i].get_socket_fd());
			cout << "Server " << i << " initialized on port " << servers[i].get_port() << endl;
		}
		run_multiserver(servers);
		for (size_t i = 0; i < servers.size(); ++i)
			close(servers[i].get_socket_fd());
    }
    catch (exception & e)
    {
        cerr << RED << e.what() << RESET << endl; // a revoir 
		return 1;
    }
    return 0;
}



// int main(void)
// {
// 	c_server server;
	
// 	server.create_socket();
// 	server.bind_and_listen();
// 	server.set_non_blocking(server.get_socket_fd());
	
// 	//struct sockaddr_in socket_address = server.get_socket_addr();
// 	//int socket_fd = server.get_socket_fd();
// 	//int connected_socket_fd = 0;
// 	//c_response response_handler;

// 	while (true)
// 	{
// 		//cout << "Waiting for connections..." << endl;
// 		server.setup_pollfd();
// 		server.handle_poll_events();
	
// 	}
// 	close(server.get_socket_fd());
// 	return (0);
// }
