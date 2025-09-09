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

string const &  c_parser::get_value() const
{
    return this->_current->value;
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



/*----------------------------   LOCATION   -----------------------------*/

/*---------------------   location : main function   -----------------------------*/
void                c_parser::parse_location_block(c_server & server)
{
    advance_token(); // skip "location"

    expected_token_type(TOKEN_VALUE);
    if (current_token().value[0] != '/')
        throw invalid_argument("invalid path for the location : " + current_token().value); // completer msg d'erreur -> ajout ligne
    if (current_token().value[current_token().value.length() - 1] == '/')
        location_url_directory(server);
    else
    {
        // location_file(server);
        // location directory(server);
    }
    expected_token_type(TOKEN_RBRACE);
    advance_token(); // skip RBRACE
}

/*---------------------   location : directory as value   ----------------------*/
void                c_parser::location_url_directory(c_server & server)
{
    c_location  location;

    location.set_url_key(_current->value);
    location.set_is_directory(true);
    location.set_index_files(server.get_indexes());
    location.set_body_size(server.get_body_size());
    // si pas de directive alias ne pas set la valeur avec le root
    // location.set_err_page(server.get_err_pages()); // A FAIRE

    advance_token(); // skip url location
    expected_token_type(TOKEN_LBRACE);
    advance_token(); // skip LBRACE

    while (!is_token_type(TOKEN_RBRACE) && !is_at_end())
    {
        if (is_token_type(TOKEN_DIRECTIVE_KEYWORD))
            location_directives(location);
        else if (is_token_type(TOKEN_RBRACE))
            break ;
        else
        {
            cout << "type" << _current->type << endl;
            throw invalid_argument("invalid in location = " + _current->value);
        }
    }
    server.add_location(location.get_url_key(), location);
}

/*---------------------   location : file as value   ---------------------------*/
void                c_parser::location_url_file(c_server & server)
{
    (void)server;
    // le file est a mettre comme index, 
    // pas besoin de recuperer les fichiers index listes par la directive indexes
}

/*------------------------   location : directives   ---------------------------*/
void                c_parser::location_directives(c_location & location)
{
    int     flag_cgi = 0;
    int     flag_upload = 0;

    if (is_token_value("cgi"))
    {
        flag_cgi++;
        location.clear_cgi();
        location_cgi(location);
    }
    else if (is_token_value("index"))
    {
        location.clear_indexes();   
        location_indexes(location);
    }
    else if (is_token_value("upload_path"))
    {
        flag_upload++;
        if (flag_upload > 1)
            throw invalid_argument("directive upload_path can be define just once");
        parse_upload_path(location);
    }
    else if (is_token_value("methods"))
        location_methods(location);
    else if (is_token_value("alias"))
        location_alias(location);
    else if (is_token_value("client_max_body_size"))
        parse_body_size(location);
    else if (is_token_value("autoindex"))
        parse_auto_index(location);
    
    else
        return;
}

void                c_parser::location_methods(c_location & location)
{
    vector<string>  tmp_methods;

    advance_token();
    expected_token_type(TOKEN_VALUE);

    while (is_token_type(TOKEN_VALUE))
    {
        if (get_value() == "GET" || get_value() == "POST" || get_value() == "DELETE")
        {    
            tmp_methods.push_back(get_value());
            advance_token();
        }
        else
            throw invalid_argument("wrong method ==> " + get_value());
    }
    if (tmp_methods.empty())
        throw invalid_argument("directive method can't be empty");
    location.set_methods(tmp_methods);
    expected_token_type(TOKEN_SEMICOLON);
    advance_token();
}

void                c_parser::location_cgi(c_location & location)
{
    string  extension;
    string  path;
    map<string, string> temp;

    advance_token(); // skip directive
    expected_token_type(TOKEN_VALUE);
    extension = get_value();
    advance_token(); // skip first value (suppose to be extension)
    expected_token_type(TOKEN_VALUE);
    path = get_value();
    advance_token(); // skip second value (path)
    expected_token_type(TOKEN_SEMICOLON);
    advance_token(); //skip semicolon
    
    if (extension != ".py" && extension != ".sh" && extension != ".php") // verifier toutes les extensions autorisees
        throw invalid_argument("invalid extension for the CGI ==> " + extension);
    if (path[0] != '/')
        throw invalid_argument("invalid path for the CGI ==> " + path);
    if (path[path.size() - 1] == '/')
        throw invalid_argument("invalid path for the CGI ==> " + path);
    if (!is_executable_file(path))
        throw invalid_argument("no such file or permission denied ==> " + path);
    temp[extension] = path;
    location.set_cgi(temp);
}

void                c_parser::location_indexes(c_location & location)
{
    advance_token(); // skip directive
    expected_token_type(TOKEN_VALUE);
    
    while (is_token_type(TOKEN_VALUE))
    {
        if (location.get_bool_is_directory())
        {
            location.add_index_file(_current->value);            
            advance_token();
        }
    }
    if (location.get_indexes().empty())
       throw invalid_argument("Error: index directive requires at least one value");

    expected_token_type(TOKEN_SEMICOLON);
    advance_token();
}

void                c_parser::location_alias(c_location & location)
{
    advance_token(); // skip directive
    expected_token_type(TOKEN_VALUE);
    string  alias = _current->value;
    advance_token(); // skip value
    expected_token_type(TOKEN_SEMICOLON);
    advance_token();

    if (alias[0] != '/' && alias[0] != '.')
        throw invalid_argument("invalid path for alias ==> " + _current->value);
    if (alias[alias.length() - 1] != '/')
        alias.push_back('/');
    location.set_alias(alias);
}

void        c_parser::parse_upload_path(c_location & location)
{
    advance_token();
    expected_token_type(TOKEN_VALUE);
    string path = _current->value;
    advance_token();
    expected_token_type(TOKEN_SEMICOLON);
    advance_token();
    
    if (path[0] != '/' && path[0] != '.')
        throw invalid_argument("invalid path for directive upload_path ==> " + path);
    if (path[path.length() - 1] != '/')
       throw invalid_argument("invalid path for directive upload_path ==> " + path);
    location.set_upload_path(path); 
}

void        c_parser::parse_auto_index(c_location & location)
{
    advance_token(); // skip directive
    expected_token_type(TOKEN_VALUE);
    
    if (_current->value != "ON" && _current->value != "on"
        && _current->value != "OFF" && _current->value != "off")
        throw invalid_argument("invalid value for auto_index ==> " + _current->value);
    if (_current->value == "ON" || _current->value == "on")
        location.set_auto_index(true);
    else if (_current->value != "OFF" || _current->value != "off")
        location.set_auto_index(false);
    
    advance_token(); // skip value
    expected_token_type(TOKEN_SEMICOLON);
    advance_token();
}


/*-----------------------   server : directives   ------------------------------*/

string              c_parser::parse_ip(string const & value)
{
    if (value == "*")
        return ("0.0.0.0");
    if (value.find_first_not_of("0123456789.") != string::npos
        || count(value.begin(), value.end(), '.') != 3
        || value.empty())
        throw invalid_argument("invalid IP adress ==> " + value);
    size_t  pos = 0;
    long    is_valid_ip;
    int     i = 0;
    string  buf;
    string temp = value;
    while (i < 4)
    {
        pos = temp.find('.');
        buf = temp.substr(0, pos);
        if (buf.empty())
            throw invalid_argument("invalid IP adress ==> " + value);
        is_valid_ip = strtol(buf.c_str(), NULL, 10);
        if (errno == ERANGE || is_valid_ip < 0 || is_valid_ip > 255)
            throw invalid_argument("invalid IP adress ==> " + value);
        temp.erase(0, pos + 1);
        i++;
    }
    return value;
}

void                c_parser::parse_listen_directive(c_server & server)
{
    long    port = -1;
    string  str_ip;

    advance_token(); // skip keyword "listen"

    if (get_value().find_first_not_of("0123456789") == string::npos)
    {
        port = strtol(get_value().c_str(), NULL, 10);
        str_ip = "0.0.0.0"; 
        // si pas de precision -> ecouter sur toutes les interfaces disponibles 
        // (toutes les adresses IP locales en meme temps)
        // 127.0.0.1 --> ecoute uniquement sur localhost (acces seulement depuis notre machine)
    }
    else
    {
        string  str_port;
        if (count(get_value().begin(), get_value().end(), ':') != 1)
            throw invalid_argument("invalid port ==> " + get_value());
        str_port = get_value().substr(get_value().find(':') + 1, get_value().size());
        if (str_port.empty())
            throw invalid_argument("invalid port ==> " + get_value());
        if (str_port.find_first_not_of("0123456789") != string::npos)
            throw invalid_argument("invalid port ==> " + get_value());
        port = strtol(str_port.c_str(), NULL, 10);
        str_ip = parse_ip(get_value().substr(0, get_value().find(':')));
    }
    if (port == ERANGE || port < 0 || port > 65535)
        throw invalid_argument("invalid port [0-65535] ==> " + get_value());
    server.set_port(static_cast<uint16_t>(port));
    server.set_ip(str_ip);

    advance_token(); // skip ip+port
    expected_token_type(TOKEN_SEMICOLON);
    advance_token(); // skip semicolon
}

void                c_parser::parse_index_directive(c_server & server)
{
    vector<string>  index_files;
    advance_token(); // skip keyword "index"
    expected_token_type(TOKEN_VALUE);

    while (is_token_type(TOKEN_VALUE))
    {
        index_files.push_back(current_token().value);
        advance_token();
    }
    if (index_files.empty())
       throw invalid_argument("Error: index directive requires at least one value");
    // POUR VERIFIER LA VALIDITE DU FICHIER
    // vector<string>::iterator it = index_files.begin();
    // while (it != index_files.end())
    // {
    //     if (is_executable_file(*it))
    //     {
    //         valid_file = *it;
    //         break ;
    //     }
    //     it++;
    // }
    // if (valid_file.empty())
    //     throw invalid_argument("Error in index directive: there is no valid file");
    server.set_indexes(index_files);
    expected_token_type(TOKEN_SEMICOLON);
    advance_token(); // skip semicolon
}

void                c_parser::parse_server_name(c_server & server)
{
    vector<string>  temp_names;
    advance_token(); // skip keyword "server_name"
    expected_token_type(TOKEN_VALUE);

    while (is_token_type(TOKEN_VALUE))
    {
        temp_names.push_back(current_token().value);
        advance_token();
    }
    if (temp_names.empty())
        throw invalid_argument("server_name directive requires at least one value");
    server.set_name(temp_names);
    expected_token_type(TOKEN_SEMICOLON);
    advance_token(); // skip semicolon
}

void                c_parser::parse_server_directives(c_server & server)
{
    if (is_token_value("index"))
        parse_index_directive(server);
    else if (is_token_value("listen"))
        parse_listen_directive(server);
    else if (is_token_value("server_name"))
        parse_server_name(server);
    else if (is_token_value("client_max_body_size"))
        parse_body_size(server);
    else /* a enlever / reprendre */
        advance_token();
    // cout << "parse index directive = " << this->_current->value << endl;

    // else if (is_token_value("error_page"))
    // else if (is_token_value("client_max_body_size"))
    // else
    //     throw invalid_argument("Unknown server directive: " + current_token().value);

    // /!\ certaines dir prennent plusieurs values
}


/*-------------------   parse server block ---------------------*/

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
            parse_location_block(server);
            has_location = true;
            // server::_locations
            // c_location = parse_location_block();
            // server.add_location(location.get_path(), location);
            
            // cout << "ICI" << endl;
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
        // else if (is_token_type(TOKEN_EOF))
        // {
        //     cout << "TOKEN_EOF " << endl;
        //     break;
        // }
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


/*-----------------------   utils -----------------------*/

size_t            c_parser::convert_to_octet(string const & str, string const & suffix, size_t const i) const
{
    size_t limit = 0;
    string result;

    if (suffix.empty())// pas de suffix donc chiffre deja en octet
        limit = strtol(str.c_str(), NULL, 10); 
    else
    {
        result = str.substr(0, i);
        limit = strtol(str.c_str(), NULL, 10);
    }
    if (errno == ERANGE)
        throw invalid_argument("invalid argument for max_body_size (conversion of the number) ==> " + str);
    
    if (!suffix.empty())
    {
        if (suffix == "k" || suffix == "K")
            limit = limit * 1024;
        else if (suffix == "m" || suffix == "M")
            limit = limit * 1024 * 1024;
        else if (suffix == "g" || suffix == "G")
            limit = limit * 1024 * 1024 * 1024;
        else
            throw invalid_argument("invalid argument for max_body_size (conversion of the number) ==> " + str);
    }

    return limit;
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

