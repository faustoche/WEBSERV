#pragma once

/*-----------  INCLUDES -----------*/

#include <poll.h>
#include "server.hpp"
#include <time.h>

/*-----------  DEFINE -----------*/

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

public:
	client();
	~client();
};
