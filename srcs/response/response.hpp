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
class	c_client;
class	c_cgi;
class	c_location;

struct	s_multipart // pour la gestion de POST pour l'upload de fichiers
{
	string	name; // description, file
	string	filename; // vide si champ texte
	string	content_type; // vide si champ texte
	string	content; // donnee (texte ou binaire) 
	bool	is_file;
};

class c_response
{
private:
	string _response;
	string _file_content;

	/***** AC *****/
	c_server&			_server;
	map<string, string> _headers_response;
	string				_body;
	int					_client_fd;
	int					_status;
	bool				_is_cgi;
	bool				_error;
	c_client			&_client;

public:

	void define_response_content(const c_request &request);
	// void define_response_content(c_request &request, c_server &server, int client_fd);
	const string &get_response() const { return (_response); };
	const string &get_file_content() const { return (_file_content); }

	/****** AC *****/
	c_response(c_server& server, c_client &client);
	~c_response();
	void			set_header_value(const string &key, const string &value) { _headers_response[key] = value; };
	const string 	&get_header_value(const string& key) const;
	void			set_body(const string &body) { this->_body = body; };
	void			set_status(const int &status) { this->_status = status; };
	int				get_status() { return this->_status; };
	const string	&get_body() { return this->_body; };
	bool			get_is_cgi() { return this->_is_cgi; };
	bool			get_error() { return this->_error; };
	const int		&get_client_fd() { return this->_client_fd; };

	void			clear_response();
	void			set_error() { this->_error = true; };
	void			build_error_response(int error_code, const string version, const c_request &request);

private:
	string	read_error_pages(int error_code);
	void	build_success_response(const string &file_path, const string version, const c_request &request);
	void    build_cgi_response(c_cgi & cgi, const c_request &request);
	// void	build_error_response(int error_code, const string version, const c_request &request);
	void	build_redirect_response(int code, const string &location, const string &version, const c_request &request);
	void	build_directory_listing_response(const string &dir_path, const string &version, const c_request &request);
	string	load_file_content(const string &file_path);
	string	get_content_type(const string &file_path);

	/***** POST method *****/
	void						handle_post_request(const c_request &request, c_location *location, const string &version);
	map<string, string> const	parse_form_data(const string &body);
	string const				url_decode(const string &body);
	void						create_form_response(const map<string, string> &form, const c_request &request, const string &version);
	void						handle_test_form(const c_request &request, const string &version);
	void						handle_contact_form(const c_request &request, const string &version);
	bool						save_contact_data(const map<string, string> &data);
	void						error_form_response(const string &msg, const c_request &request);
	void						handle_upload_form_file(const c_request &request, const string &version);
	void 						load_todo_page(const string &version, const c_request &request);
	void						handle_todo_form(const c_request &request, const string &version);
	void						handle_upload_form_file(const c_request &request, const string &version, c_location *location);
	vector<s_multipart> const	parse_multipart_data(const string &body, const string &boundary); // return une reference ?
	s_multipart const			parse_single_part(const string &raw_part);
	void						parse_header_section(const string &header_section, s_multipart &part);
	string						extract_line(const string &header_section, const size_t &pos);
	string						extract_quotes(const string &line, const string &type);
	string						extract_after_points(const string &line);
	string						extract_boundary(const string &content_type);
	string						save_uploaded_file(const s_multipart &part, c_location *location);
	string 						extract_extension(const string &filename, string &name, c_location *location);
	void						buid_upload_success_response(const string &file_path, const string version, const c_request &request);
	string						sanitize_filename(const string &filename, c_location *location);
	/***** DELETE method *****/
	void						handle_delete_todo(const c_request &request, const string &version);
	void						handle_delete_request(const c_request &request, const string &version, string file_path);
	void						load_upload_page(const string &version, const c_request &request);
	void						handle_delete_upload(const c_request &request, const string &version);

	/***** DELETE method *****/
	void						handle_put_request(const c_request &request, c_location *location, const string &version, const string &file_path);
};
