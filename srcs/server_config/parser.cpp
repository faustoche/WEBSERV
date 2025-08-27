#include "parser.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_parser::c_parser(string const file) : c_lexer(file), _error_msg("")
{
    _it = c_lexer::_tokens.begin();
    vector<c_server> servers = parse(); // liste des servers si plusieurs

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
        //revoir si ce ne serait pas mieux de modif le message d'erreur
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
            // c_server server = parse_server_block();
            // servers.push_back(server);
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
        return vector<c_server>();
    }
    
}

