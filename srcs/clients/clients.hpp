#pragma once

/*-----------  INCLUDES -----------*/

#include <poll.h>
#include "server.hpp"
#include <time.h>

/*-----------  DEFINE -----------*/

/* On attribue un état particulier à chaque état du client pour avoir un vrai suivi et pouvoir gérer les timeouts et autres soucis */


enum class client_state { READING, PROCESSING, SENDING, DISCONNECTED };

/*-----------  CLASS -----------*/

// 1 fd pour la lecture
// 1 varialbe pour l'etat du client cf enum
// 1 taille du buffer
// 1 timestamps pour g2rer le timeout

class c_client
{
private:
	string	_fd;
	string	_state;
	char	_buffer;
	struct timeval _timestmps;
	map<int, c_client> _clients; // stocker les clients actifs / int = fd du socket

public:
	c_client add_client();
	c_client remove_client();
	c_client find_client();

};
