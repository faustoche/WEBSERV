#include "server.hpp"
#include "werbserv_config.hpp"

volatile sig_atomic_t g_terminate = 1;

void handle_stop(int sig)
{
	(void)sig;
	g_terminate = 0;
}

void handle_sigpipe(int sig) { if(sig) {}}

/* Checking the flags and adding the non-blocking flags */

void c_server::set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
	{
		cerr << "Error: F_GETFL - " << errno << endl;
		return ;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		cerr << "Error: F_SETFL - " << errno << endl;
		return ;
	}
}

/* Launching the main loop */

void	run_multiserver(vector<c_server> &servers)
{	
	while (g_terminate)
	{
		bool has_fatal_error = false;
		for (size_t i = 0; i < servers.size(); ++i)		
		{
			servers[i].setup_pollfd();
			servers[i].handle_poll_events();
			if (servers[i].get_fatal_error())
			{
				has_fatal_error = true;
				break ;
			}
		}
		if (has_fatal_error)
		{
			cout << PINK << "[ERROR] Poll failed. Stopping all servers." << RESET << endl;
			break ;
		}
	}
	for (size_t i = 0; i < servers.size(); ++i)	
		servers[i].close_all_sockets_and_fd();
}

int main(int argc, char **argv)
{
	signal(SIGINT, handle_stop);
	signal(SIGTERM, handle_stop);
	signal(SIGPIPE, handle_sigpipe);

	try
	{
		if (argc != 2)
			throw invalid_argument("The program must have one argument (the configuration file)");

		c_webserv_config webserv(argv[1]);
	
		vector<c_server> servers = webserv.get_servers();
		if (servers.empty())
			throw invalid_argument("Error: No servers configurations");

		for (size_t i = 0; i < servers.size(); ++i)
		{
			servers[i].create_socket_for_each_port(servers[i].get_ports());
			vector<int> ports = servers[i].get_ports();
			for (vector<int>::const_iterator it = ports.begin(); it != ports.end(); ++it)
			{
				cout << YELLOW << "🐶 Server " << i << " created and listening on port " << *it << RESET << std::endl;
			}
		}
		run_multiserver(servers);
	}
	catch (exception & e)
	{
		cerr << RED << e.what() << RESET << endl;
		return 1;
	}
	return 0;
}
