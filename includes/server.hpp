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

#define	BUFFER_SIZE 1024
using	namespace std;
