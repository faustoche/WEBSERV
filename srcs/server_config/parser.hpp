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

class  c_parser : public c_lexer {

public :
	c_parser(string const file);
	~c_parser();

	vector<c_server>	parse();
	s_token				current_token() const;
	string const &		get_value() const;
	s_token				peek_token() const;
	void				advance_token();
	bool				is_at_end() const;

private :
	vector<s_token>::iterator	_current;
	string						_error_msg;

	vector<c_server>	parse_config();
	c_server			parse_server_block();
	void				parse_location_block(c_server & server);
	void				parse_server_directives(c_server & server);
	void				parse_index_directive(c_server & server);
	void				parse_listen_directive(c_server & server);
	void				parse_root_directive(c_server & server);
	void				parse_server_name(c_server & server);
	string				parse_ip(string const & value);
	void				parse_server_cgi(c_server & server);
	void				location_url_directory(c_server & server);
	void				location_url_file(c_server & server);
	void				location_directives(c_location & location);
	void				parse_alias(c_location & location);
	void				parse_cgi(c_location & location);
	void				location_indexes(c_location & location);
	void				parse_methods(c_location & location);
	void				parse_auto_index(c_location & location);
	void				parse_upload_path(c_location & location);
	void				parse_redirect(c_location & location);
	void				parse_allowed_extensions(c_location & location);
	void				parse_allowed_data_dir(c_location & location);

	void				loc_parse_error_page(c_location & location);
	
	void				expected_token_type(int expected_type) const;
	bool				is_token_value(std::string key);
	bool				is_token_type(int type);
	size_t				convert_to_octet(string const & str, string const & suffix, size_t const i) const;
	
	void	throw_error(string const & first, string const & second, string const & value);

	template<typename C>
	void	parse_body_size(C & servloc);
	template<typename C>
	void	parse_error_page(C & servloc);
};

template<typename C>
void            c_parser::parse_body_size(C & servloc)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);
	string  str = _current->value;
	advance_token();
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

	if (limit == 0)
		throw invalid_argument("invalid argument for max_body_size, it can't be unlimited value => " + str);

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
	path = _current->value;
	servloc.add_error_page(codes, path);

	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

string      my_to_string(int int_str);
