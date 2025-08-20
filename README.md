This is - WEBSERV

-----------------------------------------------------------------

*Formatage du code*

Préfixage des types de données personnalisées:
Préfixage des types de données personnalisées :
	- typedef -> t_ ( exemple : t_location )
	- struct -> s_ ( exemple : s_location )
	- class -> c_ ( exemple : c_location )
	- Pointeur sur fonction f_ (exemple f_parser)

Utilisation des attributs privés de nos classes :
Prefixage par _ ( exemple -> au lien de this->name -> _name )

Les fonctions doivent (quand c'est possible) se prefixer par une action.
Exemples :
	- Une fonction qui va chercher une donnée peut se prefixer par get_. (get_name())
	- Une fonction qui retourne un booléen peut se prefixer par is_. (is_odd())