#pragma once

/*-----------  INCLUDES -----------*/

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#include "response.hpp"
#include "request.hpp"
#include "colors.hpp"

/*-----------  DEFINE -----------*/

#define	BUFFER_SIZE 4096
using	namespace std;

string int_to_string(int n);

class server
{
private:

public:
	server();
	~server();

	void create_socket();
	void bind_and_listen();
	void set_non_blocking();
};
