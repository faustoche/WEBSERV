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
            // locations directives
            void                location_cgi(c_location & location);
            void                location_indexes(c_location & location);
            void                location_methods(c_location & location);
            void                location_body_size(c_location & location);

            // utils
            void                expected_token_type(int expected_type) const;
            bool                is_token_value(std::string key);
            bool                is_token_type(int type);
            bool                is_executable_file(const std::string & path);

            // void    expected_token_value(int expected_type) const;

            // error handling
            bool            has_error() const;
            string const &  get_error() const;
            void            clear_error();
};

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
