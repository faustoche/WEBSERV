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

		/************* AC *************/
		const string& get_url_key() const { return _url_key; };
		const string& get_location_root() const { return _location_root; };
		const map<string, string>& get_cgi_extension() const { return _cgi_extension; };
		void	set_url_key(string new_key) { _url_key = new_key; };
		void	set_location_root(string new_root) { _location_root = new_root; };
		void	set_cgi_extension(map<string, string> cgi_ext) { _cgi_extension = cgi_ext; };
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
};



