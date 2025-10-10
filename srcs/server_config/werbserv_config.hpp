#pragma once

/*-----------  INCLUDES -----------*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include "parser.hpp"
#include "server.hpp"

using namespace std;

/*-----------  CLASS -----------*/

class c_webserv_config
{
public:
	c_webserv_config(string const & file);
	~c_webserv_config();

	bool				load_configuration();
	vector<c_server> &	get_servers();
	void				print_configurations();

private:
	string				_config_files;
	vector<c_server>	_servers;
};



