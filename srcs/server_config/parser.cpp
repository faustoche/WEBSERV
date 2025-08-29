#include "parser.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_parser::c_parser(string const file) : c_lexer(file), _error_msg("")
{
    _current = c_lexer::_tokens.begin();
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
    return *(this->_current);
}

s_token c_parser::peek_token() const
{
    if (this->_current + 1 >= c_lexer::_tokens.end())
        return s_token(TOKEN_EOF, "", 0, 0);
    return *(this->_current + 1);
}

void    c_parser::advance_token()
{
    if (!is_at_end())
        this->_current++;
}

bool    c_parser::is_at_end() const
{
    return this->_current >= c_lexer::_tokens.end() || this->_current->type == TOKEN_EOF;
}

/*-----------------   check token -------------------*/

void    c_parser::expected_token_type(int expected_type) const
{
    if (expected_type != this->_current->type)
    {
        string error_msg = "Parse error at line " + my_to_string(_current->line) +
                          ", column " + my_to_string(_current->column) +
                          ": Expected token type " + my_to_string(expected_type) +
                          ", but got '" + _current->value + "'";
        throw invalid_argument(error_msg);
    }
}

bool    c_parser::is_token_value(std::string key)
{
    if (this->_current->value == key)
        return true;
    return false;
}

bool    c_parser::is_token_type(int type)
{
    if (this->_current->type == type)
        return true;
    return false;
}


/*-----------------------   utils   ----------------------*/

string      my_to_string(int int_str)
{
    ostringstream stream_str;
    stream_str << int_str;
    string  str = stream_str.str();
    return str;

}

bool                c_parser::is_executable_file(const std::string & path)
{
    return access(path.c_str(), X_OK) == 0;
}

// bool                c_parser::is_valid_port(string & port_str)
// {
//     try
//     {
//         int port = stoi(port_str);
//         return (port > 0 && port <= 65535);
//     }
//     catch (...)
//     {
//         return false ;
//     }
// }

// bool                c_parser::is_valid_path(string & const path)
// {
//     // revoir
//     return !path.empty() && (path[0] == '/' || path.find("./") == 0);
// }

/*-------------------   parse directives   ----------------------*/

void                c_parser::parse_index_directive(c_server & server)
{
    advance_token(); // skip keyword "index"

    // Ngnix permet plusieurs fichiers index --> il faut prendre le premier qui fonctionne
    vector<string>  index_files;
    string          valid_file = "";

    while (is_token_type(TOKEN_VALUE))
    {
        index_files.push_back(current_token().value);
        advance_token(); // a la fin devrait etre sur token ";"
    }
    if (index_files.empty())
       throw invalid_argument("Error: index directive requires at least one value");
    
    vector<string>::iterator it = index_files.begin();
    while (it != index_files.end())
    {
        if (is_executable_file(*it))
        {
            valid_file = *it;
            break ;
        }
        cout << "it = " << *it << endl;
        it++;
    }
    if (valid_file.empty())
        throw invalid_argument("Error in index directive: there is no valid file");
    server.set_index_file(valid_file);
    expected_token_type(TOKEN_SEMICOLON);
    advance_token(); // skip semicolon
}

void                c_parser::parse_server_directives(c_server & server)
{
    if (is_token_value("index"))
        parse_index_directive(server);
    else /* a enlever / reprendre */
        advance_token();
    // cout << "parse index directive = " << this->_current->value << endl;
    // else if (is_token_value("server_name"))
    // else if (is_token_value("listen"))
    // else if (is_token_value("error_page"))
    // else if (is_token_value("client_max_body_size"))
    // else
    //     throw invalid_argument("Unknown server directive: " + current_token().value);

    // /!\ certaines dir prennent plusieurs values
}


/*-------------------   parse block ---------------------*/

void                c_parser::parse_location_block(c_server & servre)
{
    advance_token(); // skip "location"
    expected_token_type(TOKEN_VALUE);
    if (current_token().value[0] != '/')
        throw invalid_argument("invalid path for the location : " + current_token().value); // completer msg d'erreur -> ajout ligne
    // parser le path, les differentes directives
    
}

c_server            c_parser::parse_server_block()
{
    c_server    server;
    bool        has_location = false;

    advance_token(); // skip "server"
    expected_token_type(TOKEN_LBRACE);
    advance_token(); // skip "lbrace"
    while (!is_token_type(TOKEN_RBRACE) && !is_at_end())
    {
        if (is_token_type(TOKEN_DIRECTIVE_KEYWORD))
        {
            // directives server doivent etre avant les blocs location
            if (has_location)
                throw invalid_argument("Error: server directive is forbidden after location block"); // + *(_current)->value
            parse_server_directives(server);
        }
        else if (is_token_type(TOKEN_BLOC_KEYWORD) && is_token_value("location"))
        {
            // parse_location_block();
            has_location = true;
            // server::_locations
            // c_location = parse_location_block();
            // server.add_location(location.get_path(), location);
        }
        else
        {
            cout << "invalid argument" << endl;
            throw invalid_argument("Unexpected token in server block: " + current_token().value);
        }
    }

    expected_token_type(TOKEN_RBRACE);
    advance_token();

    // if (!server.is_valid())
    //     throw invalid_argument("Invalid server configuration: " + server.get_validation_error());
    return server;

}

/*-------------------   loop method ---------------------*/

vector<c_server>    c_parser::parse_config()
{
    vector<c_server> servers;

    while (!is_at_end())
    {
        s_token token = current_token();
        if (is_token_value("server") && is_token_type(TOKEN_BLOC_KEYWORD)) // parser un par un les block server
        {
            // cout << "SERVER_BLOCK " << endl;
            c_server server = parse_server_block();
            servers.push_back(server);
        }
        else if (is_token_type(TOKEN_EOF))
        {
            cout << "TOKEN_EOF " << endl;
            break;
        }
        else
        {
            string error_msg = "Unexpected token at line " + my_to_string(token.line) +
                                ", column " + my_to_string(token.column) + ": " + token.value;
            throw invalid_argument(error_msg);
            break;
        }
    }
    if (servers.empty())
        throw invalid_argument("Configuration file must contain at least one server block");
    return servers;
}

/*-----------------   principal method -------------------*/
vector<c_server>    c_parser::parse()
{
    try
    {
        return parse_config();
    }
    catch (std::exception & e)
    {
        _error_msg = e.what();
        cerr << _error_msg << endl;
        return vector<c_server>(); // revoir
    }

}

/*-----------------   error handling -------------------*/

bool    c_parser::has_error() const
{
    return !_error_msg.empty();
}

string const &  c_parser::get_error() const
{
    return _error_msg;
}

void    c_parser::clear_error()
{
    _error_msg.clear();
}

