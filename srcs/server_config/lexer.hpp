#pragma once

/*-----------  INCLUDES -----------*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

using namespace std;


/*------------- ENUM ------------*/
/* definit la categorie gramaticale 
de chaque token */

enum    token_type {
	TOKEN_BLOC_KEYWORD, // server, location
	TOKEN_DIRECTIVE_KEYWORD, // listen, root, index, ...
	TOKEN_VALUE,      // /var/www, index.html, 8080, ...
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_SEMICOLON,
	TOKEN_EOF,
	TOKEN_ERROR
};

/*-----------  STRUCT -----------*/

struct  s_token {
	token_type      type;  // categorie gramaticale
	string          value; // contenu reel du token --> texte
	int             line;
	int             column;

	s_token(token_type t, string const & v, int l, int c)
	: type(t), value(v), line(l), column(c) {}

	s_token() : type(TOKEN_ERROR), value(""), line(0), column(0){}
};


/*-----------  CLASS -----------*/

class  c_lexer {

public :
	c_lexer(string const filename);
	~c_lexer();

	void	get_next_token();
	// s_token peek_token();
	bool	has_more_token();

	// getters
	int			get_current_line() const;
	int			get_current_column() const;
	vector<s_token>&	get_list_of_token();

	 //remettre en private

protected :
	
	string		_content; // contenu entier du .conf
	size_t		_pos; // position actuelle dans _content
	int		_current_line;
	int		_current_column;

	vector<s_token>	_tokens;

	// private method
	bool	open_file(string const & filename);

	// navigation dans le contenu
	char		current_char() const;
	bool		is_at_end() const;
	string		read_word();
	void		skip_whitespace();
	void		skip_line();
	void		advance();

	// pour tokens
	void	init_list_of_tokens();
	bool	has_more_tokens() const;
	bool	is_bloc_keyword(string const & word) const;
	bool	is_directive_keyword(string const & word) const;

};


