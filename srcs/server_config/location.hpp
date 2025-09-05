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
		c_location(/* args */);
		~c_location();

		c_location const& operator=(c_location const& rhs);

		// Setters
		void				set_url_key(string const & url) {this->_url_key = url; };
		void				set_root(string const & root) {this->_location_root = root; };
		void				set_index_files(vector<string> const & files) {this->_location_index_files = files; };
		void				set_methods(vector<string> const & methods) {this->_location_methods = methods; };
		void				set_body_size(size_t const & size) {this->_location_body_size = size; };
		void				set_auto_index(bool const & index) {this->_auto_index = index; };
		void				set_redirect(pair<int, string> redirect) {this->_redirect = redirect; };
		void				set_cgi(map<string, string> cgi) {this->_cgi_extension = cgi; };
		void				set_upload_path(string const & path) {this->_upload_path = path; };
		void				set_is_directory(bool const & dir) {this->_is_directory = dir;};

		// Getters
		string const &		get_url_key() const {return _url_key; };
		string const &		get_root() const {return _location_root; };
		vector<string>		get_indexes() const {return _location_index_files; };
		vector<string>		get_methods() const {return _location_methods; };
		size_t				get_body_size() const {return _location_body_size; };
		bool				get_auto_index() const {return _auto_index; };
		pair<int, string>	get_redirect() const {return _redirect; };
		map<string, string>	get_cgi() const {return _cgi_extension; };
		string const &		get_upload_path() const {return _upload_path; };
		bool				get_bool_is_directory() const {return _is_directory; };

		void				print_location() const;

		void				clear_cgi();

		/************* AC *************/
		// c_location(c_location const& other);
		
		const map<string, string>& get_cgi_extension() const { return _cgi_extension; };
		void	set_cgi_extension(const map<string, string>& cgi_ext) { _cgi_extension = cgi_ext; };

		/******************************/

private:
		string				_url_key; // cle de la map dans le server
		string				_location_root; // ou alias --> racine des fichiers pour cette location (si non definit, herite du root du serveur)
		vector<string>		_location_index_files; // liste des fichiers index possibles
		vector<string>		_location_methods; // methodes HTTP autorisees (GET, POST, DELETE)
		size_t				_location_body_size; // taille max de requete, herite du client_max_body_size du serveur si absent
		bool				_auto_index; // activer/desactiver listing de dossier
		pair<int, string>	_redirect; // code + URL (301 /new_path)
		map<string, string>	_cgi_extension; // chemin ver lexecutable CGI
		string				_upload_path; // chemin de stockage pour les fichiers uploades
		bool				_is_directory;
};
