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

/* utilise les tokens du lexer pour construire les objetcs c_server */
class  c_parser : public c_lexer {

public :
            c_parser(string const file);
            ~c_parser();

            // tokens
            s_token current_token() const;
            s_token peek_token() const;
            void    advance_token();
            bool    is_at_end() const;
            
private :
            vector<s_token>::iterator   _it;
            string                      _error_msg;

            // principal method
            vector<c_server>    parse();
            vector<c_server>    parse_config();

            void    expected_token_type(int expected_type) const;
            // void    expected_token_value(int expected_type) const;

            // utils methods
            // bool is_valid_port
            // bool is_valid_path
            // convert to nb
};

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