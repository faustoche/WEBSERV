#pragma once

/*-----------  INCLUDES -----------*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include "lexer.hpp"
#include "server.hpp"

using namespace std;

/*-----------  CLASS -----------*/

class c_location
{
public:
		c_location();
		~c_location();
		c_location const&	operator=(c_location const & rhs);

		void								set_url_key(string const & url) {this->_url_key = url; };
		void								set_alias(string const & root) {this->_location_root = root; };
		void								set_index_files(vector<string> const & files) {this->_location_indexes = files; };
		void								add_index_file(string const & file);

		void								set_methods(vector<string> const & methods) {this->_location_methods = methods; };
		void								set_body_size(size_t const & size) {this->_location_body_size = size; };
		void								set_auto_index(bool const & index) {this->_auto_index = index; };
		void								set_redirect(pair<int, string> redirect) {this->_redirect = redirect; };
		void								set_cgi(string extension, string path);
		void								set_upload_path(string const & path) {this->_upload_path = path; };
		void								set_is_directory(bool const & dir) {this->_is_directory = dir; };	
		void								set_err_pages(map<int, string> err_pages) {this->_location_err_pages = err_pages; };
		void								add_error_page(vector<int> const & codes, string path);
		void								add_allowed_extension(const string & extension);
		
		string const &						get_url_key() const {return _url_key; };
		string const &						get_alias() const {return _location_root; };
		vector<string> const &				get_indexes() const {return _location_indexes; };
		vector<string> const &				get_methods() const {return _location_methods; };
		size_t								get_body_size() const {return _location_body_size; };
		bool								get_auto_index() const {return _auto_index; };
		pair<int, string> const &			get_redirect() const {return _redirect; };
		map<string, string>	const&			get_cgi() const {return _cgi_extension; };
		string			  					extract_interpreter(string const& extension) const;
		string const &						get_upload_path() const {return _upload_path; };
		bool								get_bool_is_directory() const {return _is_directory; };
		map<int, string> const &			get_err_pages() const {return (_location_err_pages); };
		vector<string> const &				get_allowed_extensions() const {return _allowed_extensions; };

		// Print
		void								print_location() const;
		void								print_indexes() const;
		void								print_methods() const;
		void								print_error_page() const;
		void								print_cgi() const;
		void								print_allowed_extensions() const;

		void								clear_cgi();
		void								clear_indexes();
		void								clear_err_pages();

private:
		string								_url_key;
		string								_location_root;
		vector<string>						_location_indexes;
		vector<string>						_location_methods;
		size_t								_location_body_size;
		bool								_auto_index;
		pair<int, string>					_redirect;
		map<string, string>					_cgi_extension;
		string								_upload_path;
		bool								_is_directory;
		map<int, string>					_location_err_pages;
		vector<string>						_allowed_extensions;
};
