#include "server.hpp"

/*------------------  CONSTRUCTORS AND DESTRUCTORS  -------------------*/
c_server::c_server()
{}

c_server::~c_server()
{
}

/*---------------------   CONFIGURATION FILE   ----------------------*/

/*-------------------------   setters   -----------------------------*/

void	c_server::set_index_file(string const & index)
{
	this->_index = index;
}

void	c_server::set_port(uint16_t const & port)
{
	this->_port = port;
}

void	c_server::set_ip(string const & ip)
{
	this->_ip = ip;
}

void	c_server::add_location(string const & path, c_location const & loc)
{
	if (path.empty())
		throw invalid_argument("Path for location is empty");
	// if (is_valid(loc)) //verifier si location est valide
	// 	throw invalid_argument("Invalid location");
	this->_location_map[path] = loc;
}

/*-------------------------   setters   -----------------------------*/

void	c_server::set_active_cgi(int key_fd, c_cgi* cgi)
{
	if (!cgi)
		return ;

	// Verifie si la cle existe
	map<int, c_cgi*>::iterator it = _active_cgi.find(key_fd);
	if (it != _active_cgi.end())
		it->second = cgi;
	else
		_active_cgi.insert(make_pair(key_fd, cgi));
}



/*-------------------------   debug   -----------------------------*/
void	c_server::print_config() const
{
	cout << "Index file: " << get_index_file() << endl
			<< "IP adress: " << get_ip_adress() << endl
			<< "Port: " << get_port() << endl;

	map<string, c_location>::const_iterator it;

	for (it = get_location().begin(); it != get_location().end(); it++)
	{
		it->second.print_location();
	}
			

}
