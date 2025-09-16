#pragma once

/*-----------  INCLUDES ------------*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cctype>
#include "lexer.hpp"
#include "server.hpp"

using namespace std;

/*------------  DEFINE -------------*/
#define MY_SIZE_MAX static_cast<size_t>(-1)

/*-------------  CLASS -------------*/

/* utilise les tokens du lexer pour construire les objects c_server */
class  c_parser : public c_lexer {

public :
            c_parser(string const file);
            ~c_parser();

            // principal method
            vector<c_server>    parse();

            // tokens
            s_token         current_token() const;
            string const &  get_value() const;
            s_token         peek_token() const;
            void            advance_token();
            bool            is_at_end() const;

private :
            vector<s_token>::iterator   _current;
            string                      _error_msg;


            // loop for creation of all the servers
            vector<c_server>    parse_config();

            // blocks
            c_server            parse_server_block();
            void                parse_location_block(c_server & server);

            // directives
            void                parse_server_directives(c_server & server);
            void                parse_index_directive(c_server & server);
            void                parse_listen_directive(c_server & server);
            void                parse_server_name(c_server & server);
            string              parse_ip(string const & value);
            void                parse_server_cgi(c_server & server);

            // locations
            void                location_url_directory(c_server & server);
            void                location_url_file(c_server & server);
            void                location_directives(c_location & location);
            void                parse_alias(c_location & location);
            // locations directives
            void                parse_cgi(c_location & location); //modifier noms
            void                location_indexes(c_location & location);
            void                parse_methods(c_location & location);
            void                parse_auto_index(c_location & location);
            void                parse_upload_path(c_location & location);
            void                parse_redirect(c_location & location);
            void                loc_parse_error_page(c_location & location);

            // utils
            void                expected_token_type(int expected_type) const;
            bool                is_token_value(std::string key);
            bool                is_token_type(int type);
            size_t              convert_to_octet(string const & str, string const & suffix, size_t const i) const;

            // void    expected_token_value(int expected_type) const;

            // error handling
            void    throw_error(string const & first, string const & second, string const & value);
            // bool                has_error() const;
            // string const &      get_error() const;
            // void                clear_error();

            template<typename C>
            void                parse_body_size(C & servloc);
            template<typename C>
            void                parse_error_page(C & servloc);
};

template<typename C>
void            c_parser::parse_body_size(C & servloc)
{
    advance_token();
    expected_token_type(TOKEN_VALUE);
    string  str = _current->value;
    advance_token(); // skip value
    expected_token_type(TOKEN_SEMICOLON);
    advance_token();

    if (str.empty())
        throw invalid_argument("empty value for max_body_size");

    if (str.find_first_not_of("0123456789kKmMgG") != string::npos)
        throw invalid_argument("invalid argument for max_body_size => " + str);

    string suffix;
    size_t  i = 0;
    size_t  j = 0;
    while (isdigit(str[i]) && i < str.length())
        i++;
    if (i == 0)
        throw invalid_argument("max_body_size must start with a number => " + str);
    if (i < str.length())
    {
        suffix = str.substr(i);
        for (j = 0; j + i != str.length(); j++)
        {
            if (j >= 1)
                throw invalid_argument("invalid argument for max_body_size (only k, K, m, M, g or G accepted after the number) => " + str);
            if (suffix.find_first_not_of("kKmMgG") != string::npos)
                throw invalid_argument("invalid argument for max_body_size (only k, K, m, M, g or G accepted after the number) => " + str);

        }
    }

    size_t limit = convert_to_octet(str, suffix, i);
    servloc.set_body_size(limit);
}

template<typename C>
void            c_parser::parse_error_page(C & servloc)
{
    advance_token();
    expected_token_type(TOKEN_VALUE);

    vector<int> codes;
    string      path;

    while (is_token_type(TOKEN_VALUE) && _current->value.find_first_not_of("0123456789") == string::npos)
    {
        int nb = strtol(_current->value.c_str(), NULL, 10);
        if (nb < 300 || nb > 599)
            throw invalid_argument("Invalid argument for error code ==> " + _current->value);
        codes.push_back(nb);
        advance_token();
    }
    if (codes.empty())
        throw invalid_argument("Invalid argument for error code ==> " + _current->value);

    expected_token_type(TOKEN_VALUE);
    if (_current->value[0] != '/' && _current->value[0] != '.')
        throw invalid_argument("Invalid argument for error page ==> " + _current->value);
    // verifier l'extension utilisee ??
    path = _current->value;
    servloc.add_error_page(codes, path);

    advance_token();
    expected_token_type(TOKEN_SEMICOLON);
    advance_token();
}

string      my_to_string(int int_str);


/*
    1) parse -> parse_config -> parse server bloc -> parse server directive
                                                             --> parse listen --> parse root ...


    PATTERN POUR SERVER BLOC
    c_server    parse_server_block()
    {
        c_server server

        expected token
        advance
        expected token {
        adance
        parser_server_dir(server)

    }

    c_server parse_server_directive

    PATTERN POUR DIRECTIVES
    void    parse_listen_directive()
    {
        advance_token // skip dir
        expected_token(VALUE)
        validate_port
        config_server.set_listen(port)
        advance_token
        expected_token(SEMICOLON)
        advance
    }



*/
