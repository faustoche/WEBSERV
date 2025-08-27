#include "parser.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_parser::c_parser(string const file) : c_lexer(file), _error_msg("")
{
    _it = c_lexer::_tokens.begin();
    // vector<c_server> servers = parse(); // liste des servers si plusieurs

    // fonctions qui check si erreur

}

/*-----------------  DESTRUCTOR -------------------*/

c_parser::~c_parser()
{}


/*----------------- MEMBERS METHODS  --------------*/

/*------------   navigate through tokens --------------*/

s_token c_parser::current_token() const
{
    if (is_at_end())
        return s_token(TOKEN_EOF, "", 0, 0);
    return *(this->_it);
}

s_token c_parser::peek_token() const
{
    if (this->_it + 1 >= c_lexer::_tokens.end())
        return s_token(TOKEN_EOF, "", 0, 0);
    return *(this->_it + 1);
}

void    c_parser::advance_token()
{
    if (!is_at_end())
        this->_it++;
}

bool    c_parser::is_at_end() const
{
    return this->_it >= c_lexer::_tokens.end() || this->_it->type == TOKEN_EOF;
}

/*-----------------   check token -------------------*/

void    c_parser::expected_token_type(int expected_type) const
{
    if (expected_type != this->_it->type)
        throw invalid_argument("Error: problem with the formation of configuration file, not expected => " + _it->value); 
}


/*-----------------------   check values   ----------------------*/

// void                c_parser::check_index_files()
// {
//     // fonction qui prend les fichiers dans l'ordre et check si fichier valide
//     // si premier fichier n existe pas possible quil y ait un second fichier (check de plusieurs valeurs) 
// }

/*-------------------   parse directives   ----------------------*/

void                c_parser::parse_index_directive(c_server & server)
{
    advance_token(); // skip nom de la directive
    expected_token_type(TOKEN_VALUE);
    // check_index_files()
    // recuperer la valeur dans server
    advance_token();
    expected_token_type(TOKEN_SEMICOLON);

}

void                c_parser::parse_server_directives(c_server & server)
{
    if (this->_it->value == "root")
        parse_index_directive(server)
    // lister toutes les directives
    // parse listen IP + port
    // parse index
    // certaines dir prennent plusieurs values
}


/*-------------------   parse block ---------------------*/

c_server            c_parser::parse_server_block()
{
    c_server    server;

    // expected_token_type(TOKEN_BLOC_KEYWORD);
    advance_token();
    expected_token_type(TOKEN_LBRACE);
    advance_token();
    if (this->_it->type == TOKEN_DIRECTIVE_KEYWORD)
        parse_server_directives(server);
    /* si bloc location on ne peut plus avoir de directives serveur apres */
    // else if (this->_it->type == TOKEN_BLOC_KEYWORD)
    //     parse_server_locations(); // derriere bloc(location) value(chemin) et pas "";""
    // else if ()
    // else if ()
    // else
    //     throw invalid_argument("Error: problem with the formation of configuration file, not expected => " + _it->value);
    expected_token_type(TOKEN_RBRACE);
    return server;

}

/*-------------------   loop method ---------------------*/

vector<c_server>    c_parser::parse_config()
{
    vector<c_server> servers;

    while (!is_at_end())
    {
        s_token token = current_token();
        if (token.type == TOKEN_BLOC_KEYWORD && token.value == "server") // parser un par un les block server
        {
            c_server server = parse_server_block();
            servers.push_back(server);
        }
        else
        {
            // set_error_with_context("Unexpected token / syntax:" + token.value)
            break;
        }
    }
    return servers;
}

/*-----------------   principal method -------------------*/
vector<c_server>    c_parser::parse()
{
    try
    {
        return parse_config(); //
    }
    catch (std::exception & e)
    {
        //set_error(e.what())
        // return std::vector<c_server>
        // cout << "exception" << endl;
        return vector<c_server>();
    }
    
}

