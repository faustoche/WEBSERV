#include "server.hpp"


c_server::c_server()
{ // REVOIR ENSEMBLE LES VALEURS PAR DEFAUT
	_names.push_back("default server");
	_ip = "0.0.0.0";
	_port = 80;
	_root = "./"; // dossier courrant
	_indexes.push_back("index.html");
	_body_size = 1 * 1024; // kilo octet converti en octet ---> ou 0 ?
	// _err_pages = /* generer une page html simple si non configure */



}
c_server::~c_server()
{
	
}

c_server const &	c_server::operator=(c_server const & rhs)
{
	if (this != &rhs)
	{
		_ip = rhs._ip;
		_port = rhs._port;
		_names = rhs._names;
		_root = rhs._root;
		_indexes = rhs._indexes;
		_body_size = rhs._body_size;
		_err_pages = rhs._err_pages;
	}

	return *this;
}
/*---------------------   CONFIGURATION FILE   ----------------------*/

/*-------------------------   setters   -----------------------------*/

void	c_server::set_indexes(vector<string> const & indexes)
{
	this->_indexes = indexes;
}

void	c_server::set_port(uint16_t const & port)
{
	this->_port = port;
}

void	c_server::set_ip(string const & ip)
{
	this->_ip = ip;
}

void	c_server::set_name(vector<string> const & names)
{
	this->_names = names;
}

void	c_server::add_location(string const & path, c_location const & loc)
{
	if (path.empty())
		throw invalid_argument("Path for location is empty");
	// if (is_valid(loc)) //verifier si location est valide
	// 	throw invalid_argument("Invalid location");
	_location_map[path] = loc;
}

void	c_server::add_error_page(vecotr<int> const & codes, string path)
{
	for (size_t i = 0; i < codes.size(); i++)
		_err_pages.insert(make_pair(codes[i], path));
}

/*-------------------------   setters   -----------------------------*/




/*-------------------------   debug   -----------------------------*/

void	c_server::print_indexes() const
{
	vector<string>::const_iterator it;
	for (it = get_indexes().begin(); it != get_indexes().end(); it++)
	{
		cout << *it << " " << flush;
	}
}

void	c_server::print_names() const
{
	vector<string>::const_iterator it;
	for (it = get_name().begin(); it != get_name().end(); it++)
	{
		cout << *it << " " << flush;
	}
}


void	c_server::print_config() const
{
	cout << PINK 
			<< "	server name: " << flush;
			print_names();
			cout << endl;
	cout	<< "	IP adress: " << get_ip_adress() << endl
			<< "	port: " << get_port() << endl
			<< "	max body size: " << get_body_size() << " " << endl
			<< "	index files: " << flush;
			print_indexes();
	cout << endl
        	// << "	CGI extension (.py): " << get_cgi()[".py"] << endl
        	// << "	CGI extension (.sh): " << get_cgi()[".sh"] << endl
			
	<< RESET ;

	map<string, c_location>::const_iterator it;

	int i = 1;
	for (it = get_location().begin(); it != get_location().end(); it++)
	{
		cout << endl << GREEN 
			<< "	Location [ " << i << " ]	"
			<< it->first
			<< endl;
		it->second.print_location();
		cout << RESET;
		i++;
	}
			

}
