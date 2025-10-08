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

		// Setters
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

		// Getters			
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

private:		// remplacer "root" par alias
		string								_url_key; // cle de la map dans le server
		string								_location_root; // ou alias --> racine des fichiers pour cette location (si non definit, herite du root du serveur)
		vector<string>						_location_indexes; // liste des fichiers index possibles si l'URL correspond a un repertoire -> il faut verifier la validite de lindex au moment de la requete
		vector<string>						_location_methods; // methodes HTTP autorisees (GET, POST, DELETE)
		size_t								_location_body_size; // en octets, taille max de requete, herite du client_max_body_size du serveur si absent
		bool								_auto_index; // activer/desactiver listing de dossier --> quand l'URL correspond a un repertoire et qu'aucun fichier index n'existe
		pair<int, string>					_redirect; // code + URL (301 /new_path) --> pour gerer les return 301 /new_path
		map<string, string>					_cgi_extension; // extension + chemin vers lexecutable CGI --> si l'URL demandee correspond a un fichier avec cette extension le serveur lance l'executable correspondant
		string								_upload_path; // chemin de stockage pour les fichiers uploades recu via POST (attention si POST pas autorise dans la location l'uploiad doit etre refuse et renvoyer 405)
		bool								_is_directory;
		map<int, string>					_location_err_pages;
		vector<string>						_allowed_extensions;
};



/* 


EXEMPLE =
		
		server {
    		root ./www; //definit en dur

    		location /images/ {
    		    alias ./www/media;
    			}
		}

			

_url_key = /images/
_location_root = ./www/media


url demandee /images/photos.png -> chemin reel = ./www/media/photo.png
On utilise ALIAS -> remplace completement la partie du chemin correspondant a la location
pour construire le chemin reel sur le disque :
	real_path = _location_root + (requested_url - _url_key)
location_root --> remplacer le nom par parse_alias


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
le serveur n'a pas besoin dun fichier index ni de lister un dossier (donc pas de auto_index et de location_indexes)
il verifie directement si le fichier existe, si autorise pour methode HTTP et s'il faut executer un CGI

Schéma de résolution d’une requête HTTP

Config :

server {
    root /var/www/html;

    location /images/ {
        autoindex on;
        index gallery.html index.html;
    }

    location /images/logo.png {
        allow_methods GET;
    }
}
	
Requête GET /images/logo.png :
→ ton parser trouve d’abord location /images/logo.png (file) car c’est un match exact.
→ _is_directory = false.

Requête GET /images/banner.png :
→ pas de location /images/banner.png
→ mais location /images/ existe → _is_directory = true.

Requête GET /contact.html :
→ pas de location pour /contact.html
→ le serveur utilise son root par défaut.

Requete GET /images/logo.png HTTP/1.1
Ton serveur fait grosso modo :
Chercher un location exact (fichier)
Est-ce que /images/logo.png correspond à une clé exacte de location ?
Si oui, tu prends ce bloc (_is_directory = false).
Sinon, chercher le location le plus long qui correspond au préfixe
Est-ce qu’un bloc location /images/ existe ?
Si oui, tu prends celui-là (_is_directory = true).
Si aucun location trouvé
Tu retombes sur la config par défaut du server


/!!!!/
C’est au moment de servir une requête qu'il faut verifier :
si le chemin mappé existe vraiment,
si c’est un fichier ou un dossier,
si on a le droit d’y accéder.



*/
