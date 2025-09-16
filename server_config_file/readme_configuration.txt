# RESTE A FAIRE :

---> Definir les valeurs obligatoires a avoir dans le .conf pour lancer le serveur
---> Definir les valeurs par defaut 


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Le bloc server donne la configuration par defaut pour un host::port
chaque bloc location a l'interieur peut surcharger ou completer 
cette configuration pour une partie de l'arborescence des URLs
Pour le ROOT du serveur mettre en brut la racine par defaut "./www" dans le code

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Directives au niveau server:
# Elles configurent le bloc entier du serveur virtuel (et ses locations héritent de certaines valeurs si elles ne redéfinissent pas).

- listen → IP:PORT d’écoute
- server_name → nom(s) du serveur virtuel
- client_max_body_size → taille max du corps de requête (hérité par les locations)
- error_page → pages d’erreurs personnalisées


Directives au niveau location:
# Elles modifient le comportement d’une portion d’URL

- alias → alternative à root, change complètement la résolution du chemin
- index → fichier(s) index à servir par défaut (peut être une liste)
- methods → liste des méthodes HTTP autorisées (GET, POST, DELETE, …)
- autoindex → active/désactive le listing de répertoire
- client_max_body_size → taille max pour cette location (écrase le server)
- return → redirection (code + URL)
- cgi_extension → association extension → exécutable CGI
- upload_path → dossier où stocker les fichiers uploadés


Directives valides aux deux niveaux
# Certaines peuvent se trouver dans server et dans location :
- client_max_body_size
- error_page


    # LISTEN 
    definit l'IP et/ou le port sur lequel le serveur ecoute
    ouvrir un socket sur le port 8080 (par defaut sur toutes les interfaces reseau = 0.0.0.0:8080)
    si 127.0.0.1:8080 : ecoute uniquement sur 8080 en local et pas depuis l'exterieur
    syntaxe :
    listen 127.0.0.1:8080;
    listen 8080; 
    port seul -> toutes les interfaces

    # SERVER_NAME
    nom de domaine/ alias associes au serveur
    peut etre vide, alors le premier server defini sur un host:port est le serveur par defaut
    syntaxe :
    server_name localhost wwww.monsite.com;

    # ERROR_PAGE
    definit une page personnalisee pour certains codes d'erreur
    la cle est un code HTTP (ou plusieurs), la valeur est un chemin relatif
    syntaxe :
    error_page 404 301 345 /errors/abc.html;
    error_page 407 /errors/407.html;
    error_page 500 502 503 504 /errors.50x.html;

    # CLIENT_MAX_BODY_SIZE
    taille max d'une requete
    0 => desactive la limit
    possible une seule lettre ou pas de lettre (0, 1, 100, 10k, 10K, 1g, 1G)
    definir une max body size par defaut pour le server, possible de la changer avec la directive
    syntaxe :
    max_body_size 1M; #ou en octets

    # INDEX
    fichier par defaut a afficher si la requete est un repertoire
    l'index par defaut du server est index.html SAUF si il est change 
    avec la directive index au niveau du block server
    Ngnix permet plusieurs fichiers index --> il faut prendre le premier qui fonctionne
    si index est definit au niveau du server il s'applique partout
    si index definit dans location la valeur est definie uniquement dans cette location
    si l'utilisateur ne demande pas un fichier precis mais precise juste "/"
    par ex : "http:localhost::8080/"
    le serveur voit le chemin demande -> ./www
    puis regarde la directive "index" et sert le fichier ./www/index.html
    index www/index.html www/index2.html www/index3.html;
    
    # ALIAS
    root concatene le chemin de la location avec le chemin du fichier demande
    alias remplace completement le chemin de la location par un autre chemin

    # METHODS
    # liste des methodes HTTP acceptees pour la route
    # si methode demandee n'est pas autorisee => Bad Request
    # si la directive n'est pas utilisee alors par defaut toutes les methodes sont autorisees
    # syntaxe :
    # methods GET POST DELETE;

    # REDIRECT
    redirige la route vers une autre URL
    --- 301 -> moved permanently (Le contenu a changé d’adresse de façon permanente)
    --- 302 -> found (redirection temporaire)
    --- 307 -> temporary redirect
    --- 308 -> permanent redirect
    syntaxe :
    redirect 301 http://exemple.com
    "http://" ou "https://" ou "/"
    voir ensemble quel suffixe on accepte 

    # AUTOINDEX
    active/desactive le listing du contenu d'un repertoire (le serveur genere une page HTML listant le contenu du dossier)
    syntaxe :
    autoindex on;
    autoindex of;

    # CGI_EXTENSION
    Common Gateway Interface
    associe une extension de fichier a un binaire CGI
    Defini le bin a utilise pour les .py uniquement pour la location
	sinon la location herite de des cgi_bin du serveur,
	il n'y a pas de cgi_bin par defaut dans le serveur
    syntaxe :
    cgi_extension .php /usr/bin/php-cgi;

    Lorsque serveur doit repondre a une requete CGI :
    /usr/bin/python3 ./www/cgi-bin/hello.py
    /usr/bin/python3 = programme interpretateur (executable) CGI
    ./www/cgi-bin/hello.py = script du site web a executer

    # UPLOAD_PATH
    active l'upload pour cette route et definit ou stocker les fichiers
    syntaxe :
    upload_path /tmp/uploads;


