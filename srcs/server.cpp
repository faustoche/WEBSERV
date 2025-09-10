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

	map<string, c_location>::const_iterator it;

	for (it = get_location().begin(); it != get_location().end(); it++)
	{
		it->second.print_location();
	}
			

}


void c_server::create_socket_for_each_port(const std::vector<int>&ports)
{
	// je parcours la liste de port avec mon iterateur
		// 1. creation de la socket
		// je cree une variable socket fd = create socket
		// si la socket est invalide
			// j'affiche une erreur
			// j'affiche errno
			// je continue 
		
		// 2. configuration de l'option
		// j'applique mon option pour reutiliser l'adresse
		// si ca echoue
			// j'affiche une erreur + code d'erreur
			// je ferme le socket fd
			// je continue

		// 3. structure d'adresse
		// creer l'object socket adress type sockaddr in
		// tout initialise a 0
		// finet / hton(port), inadress any

		// 4. associer le soket a l'adresse et au port
		// resultat de la conf socket = bind
			// si echec alors errur et code d'erreur
			// fermer socker fd
			// continue
		
		// 5. mettre la socket en ecoute
		// resultat  = listen
		// si echec alors erreur et code d'erreur
		// fermer socker fd
		// continue

		// 6. non bloquand
		// set non blocking

		// 7. enregistrer le tout
	// fin de boucle
}