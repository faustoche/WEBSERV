#include "server.hpp"


/*---------------------   CONFIGURATION FILE   ----------------------*/

/*-------------------------   setters   -----------------------------*/

void	c_server::set_index_file(string const & index)
{
	this->_index = index;
}

void	c_server::set_location(string const & path, c_location const & loc)
{
	if (path.empty())
		throw invalid_argument("Path for location is empty");
	// if (is_valid(loc)) //verifier si location est valide
	// 	throw invalid_argument("Invalid location");
	_location_map[path] = loc;
}

/*-------------------------   setters   -----------------------------*/

string const &	c_server::get_index_file() const
{
	return this->_index;
}


/*-------------------------   debug   -----------------------------*/
void	c_server::print_config() const
{
	cout << "Index file configuration: " << this->_index << endl;
}
