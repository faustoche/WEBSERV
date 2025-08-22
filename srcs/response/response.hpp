#pragma once

/*-----------  INCLUDES -----------*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;

#include "server.hpp"

/*-----------  CLASS -----------*/

class c_request;

class c_response
{
private:
	string _response;

public:
	void define_response_content(const c_request &request);
	const string &get_response() const;

private:
	/* Méthodes privées pour la construction des réponses*/
	void	build_success_response(const string &file_path, const string version, const c_request &request);
	void	build_error_response(int error_code, const string version, const c_request &request);
	string	load_file_content(const string &file_path);
	string	get_content_type(const string &file_path);
};