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


private:
		string				_location_path; // cle de la map dans le server
		string				_location_root; // ou alias --> racine des fichiers pour cette location (si non definit, herite du root du serveur)
		vector<string>		_location_index_files; // liste des fichiers index possibles
		vector<string>		_location_methods; // methodes HTTP autorisees (GET, POST, DELETE)
		size_t				_location_body_size; // taille max de requete, herite du client_max_body_size du serveur si absent
		bool				_auto_index; // activer/desactiver listing de dossier
		pair<int, string>	_redirect; // code + URL (301 /new_path)
		map<string, string>	_cgi_extension; // chemin ver lexecutable CGI
		string				_upload_path; // chemin de stockage pour les fichiers uploades (script a executes)
};



