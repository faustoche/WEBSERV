#pragma once

/************ INCLUDES ************/

#include <poll.h>
#include "server.hpp"
#include <sys/time.h>

/************ DEFINE ************/

using namespace std;

enum client_state {
	READING,
	PROCESSING,
	SENDING,
	DISCONNECTED
};

/************ CLASS ************/

/* On conserve l'heure de connection du derneir client pour pouvoir définir un timeout ou identifier une inactivité*/

class c_client
{
private:
	int				_fd;
	string			_state;
	char			_buffer[4096];
	struct timeval	_timestamp;

public:
	c_client();
	c_client(int client_fd);
	~c_client();

	const int &get_fd() const { return (_fd); }
};
