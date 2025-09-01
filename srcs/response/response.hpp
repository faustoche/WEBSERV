#pragma once

/************ INCLUDES ************/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <iterator>
#include "server.hpp"

/************ DEFINE ************/

using namespace std;

/************ CLASS ************/

class c_request;
class c_server;

class c_response
{
private:
	string _response;
	string _file_content;

public:
	void define_response_content(const c_request &request);
	//void define_response_content(const c_request &request, c_server &server);
	const string &get_response() const;
	const string &get_file_content() const { return (_file_content); }

private:
	void	build_success_response(const string &file_path, const string version, const c_request &request);
	void	build_error_response(int error_code, const string version, const c_request &request);
	string	load_file_content(const string &file_path);
	string	get_content_type(const string &file_path);
};