#include "server.hpp"


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
	_location_map[path] = loc;
}

/*-------------------------   setters   -----------------------------*/




/*-------------------------   debug   -----------------------------*/
void	c_server::print_config() const
{
	cout << "Index file: " << get_index_file() << endl
			<< "IP adress: " << get_ip_adress() << endl
			<< "Port: " << get_port() << endl;
	print_location();

}
