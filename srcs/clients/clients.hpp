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

class client
{
private:
	string	_fd;
	string	_state;
	char	_buffer;
	struct timeval _timestmps;
	// vector ? pour pouvoir stocker les différents clients dans un container. 

public:
	client();
	~client();

	// ajouter un client
	// supprimer un client
	// trouver un client
	// 
};
