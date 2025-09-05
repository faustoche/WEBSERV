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

		c_location &	operator=(c_location const & rhs);

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

private:
		string				_url_key; // cle de la map dans le server
		string				_location_root; // ou alias --> racine des fichiers pour cette location (si non definit, herite du root du serveur)
		vector<string>		_location_index_files; // liste des fichiers index possibles si l'URL correspond a un repertoire
		vector<string>		_location_methods; // methodes HTTP autorisees (GET, POST, DELETE)
		size_t				_location_body_size; // taille max de requete, herite du client_max_body_size du serveur si absent
		bool				_auto_index; // activer/desactiver listing de dossier --> quand l'URL correspond a un repertoire et qu'aucun fichier index n'existe
		pair<int, string>	_redirect; // code + URL (301 /new_path) --> pour gerer les return 301 /new_path
		map<string, string>	_cgi_extension; // extension + chemin ver lexecutable CGI --> si l'URL demandee correspond a un fichier avec cette extension le serveur lance l'executable correspondant
		string				_upload_path; // chemin de stockage pour les fichiers uploades recu via POST
		bool				_is_directory;
};



/* EXEMPLE =
		
		server {
    		root ./www;

    		location /images/ {
    		    alias ./www/media;
    			}
		}

				

_url_key = /images/
_location_root = ./www/media


url demandee /images/photos.png -> chemin reel = ./www/media/photo.png
pour construire le chemin reel sur le disque :
	real_path = _location_root + (requested_url - _url_key)



EXEMPLE (chemin ou fichier):

			location /images/ {
    		    alias ./www/media;
    			}

				OU

			location /images/photo.png {
    		    alias ./www/media;
    			}

CAS D'UN DOSSIER apres "location"
Si l'url correspond a un dossier -> le serveur va chercher le fichier index du dossier
Si aucun fichier index n'existe & que auto index est active -> generer un listing du dossier 
Sinon retourner une erreur 403 ou 404

CAS D'UN FICHIER apres "location"
le serveur n'a pas besoin dun fichier index
il verifie directement si le fichier existe, si autorise pour methode HTTP et s'il faut executer un CGI

*/