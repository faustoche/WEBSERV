#pragma once

/************ INCLUDES ************/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <iterator>
#include "server.hpp"
#include "request.hpp"
#include <sys/stat.h>

/************ DEFINE ************/

using namespace std;

/************ CLASS ************/

class	c_request;
class	c_server;
class	c_cgi;

class c_response
{
private:
	string _response;
	string _file_content;

	/***** AC *****/
	map<string, string> _headers_response;
	string				_body;
	int					_status;
	bool				_is_cgi;

public:

	void define_response_content(const c_request &request);
	void define_response_content(c_request &request, c_server &server);
	const string &get_response() const { return (_response); };
	const string &get_file_content() const { return (_file_content); }

	/****** AC *****/
	c_response();
	~c_response();
	void			set_header_value(const string &key, const string &value) { _headers_response[key] = value; };
	const string 	&get_header_value(const string& key) const;
	void			set_body(const string &body) { this->_body = body; };
	void			set_status(const int &status) { this->_status = status; };
	const string	&get_body() { return this->_body; };
	bool			get_is_cgi() { return this->_is_cgi; };
	void			clear_response();

private:
	void	build_success_response(const string &file_path, const string version, const c_request &request);
	void    build_cgi_response(c_cgi & cgi, const c_request &request);
	void	build_error_response(int error_code, const string version, const c_request &request);
	void	build_redirect_response(int code, const string &location, const string &version, const c_request &request);
	void	build_directory_listing_response(const string &dir_path, const string &version, const c_request &request);
	string	load_file_content(const string &file_path);
	string	get_content_type(const string &file_path);

};
