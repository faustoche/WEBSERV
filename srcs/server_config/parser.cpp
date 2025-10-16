#include "parser.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_parser::c_parser(string const file) : c_lexer(file), _error_msg("")
{
	_current = c_lexer::_tokens.begin();
}

/*-----------------  DESTRUCTOR -------------------*/

c_parser::~c_parser(){}

/*----------------- MEMBERS METHODS  --------------*/
/*------------   navigate through tokens --------------*/

s_token	c_parser::current_token() const
{
	if (is_at_end())
		return s_token(TOKEN_EOF, "", 0, 0);
	return *(this->_current);
}

string	const &  c_parser::get_value() const
{
	return this->_current->value;
}

s_token	c_parser::peek_token() const
{
	if (this->_current + 1 >= c_lexer::_tokens.end())
		return s_token(TOKEN_EOF, "", 0, 0);
	return *(this->_current + 1);
}

void	c_parser::advance_token()
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

/*----------------------------   LOCATION   -----------------------------*/
/*---------------------   location : main function   -----------------------------*/

void		c_parser::parse_location_block(c_server & server)
{
	advance_token();

	expected_token_type(TOKEN_VALUE);
	if (_current->value[0] != '/')
		throw_error("Unexpected value for the url_key of the location block (must begin with '/'): ", "", _current->value);
	if (_current->value[_current->value.length() - 1] == '/')
		location_url_directory(server);
	else
		location_url_file(server);
	expected_token_type(TOKEN_RBRACE);
	advance_token();
}

/*---------------------   location : directory as value   ----------------------*/

void		c_parser::location_url_directory(c_server & server)
{
	c_location  location;

	location.set_url_key(_current->value);
	location.set_is_directory(true);
	location.set_index_files(server.get_indexes());
	location.set_body_size(server.get_body_size());
	location.set_err_pages(server.get_err_pages());

	advance_token();
	expected_token_type(TOKEN_LBRACE);
	advance_token();

	while (!is_token_type(TOKEN_RBRACE) && !is_at_end())
	{
		if (is_token_type(TOKEN_DIRECTIVE_KEYWORD))
			location_directives(location);
		else if (is_token_type(TOKEN_RBRACE))
			break ;
		else
			throw_error("Unexpected token in location block : ", "directive -> ", _current->value);
	}
	server.add_location(location.get_url_key(), location);
}

/*---------------------   location : file as value   ---------------------------*/

void		c_parser::location_url_file(c_server & server)
{
	c_location  location;

	location.set_url_key(_current->value);
	location.set_is_directory(false);
	location.set_body_size(server.get_body_size());
	location.set_err_pages(server.get_err_pages());

	advance_token();
	expected_token_type(TOKEN_LBRACE);
	advance_token();

	while (!is_token_type(TOKEN_RBRACE) && !is_at_end())
	{
		if (is_token_type(TOKEN_DIRECTIVE_KEYWORD))
			location_directives(location);
		else if (is_token_type(TOKEN_RBRACE))
			break ;
		else
			throw_error("Unexpected token in location block : ", "directive -> ", _current->value);
	}
	server.add_location(location.get_url_key(), location);
}

/*------------------------   location : directives   ---------------------------*/

void		c_parser::location_directives(c_location & location)
{
	int     flag_cgi = 0;
	int     flag_upload = 0;

	if (is_token_value("cgi"))
	{
		flag_cgi++;
		parse_cgi(location);
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
			throw invalid_argument("In location block, the directive upload_path can be define just once");
		parse_upload_path(location);
	}
	else if (is_token_value("error_page"))
		parse_error_page(location);
	else if (is_token_value("methods"))
		parse_methods(location);
	else if (is_token_value("alias"))
		parse_alias(location);
	else if (is_token_value("client_max_body_size"))
		parse_body_size(location);
	else if (is_token_value("autoindex"))
		parse_auto_index(location);
	else if (is_token_value("redirect"))
		parse_redirect(location);
	else if (is_token_value("allowed_extensions"))
		parse_allowed_extensions(location);
	else if (is_token_value("allowed_data_dir"))
		parse_allowed_data_dir(location);
	else
		return;
}

void	c_parser::parse_allowed_data_dir(c_location & location)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);
	string  allowed_dir = _current->value;
	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();

	if ((allowed_dir[0] != '.' || allowed_dir[1] != '/') && allowed_dir[0] != '.' && allowed_dir[0] != '/')
		throw invalid_argument("Invalid path for allowed_data_dir directive): " + allowed_dir + " (must begin with './' or '/' )");
	if (allowed_dir.find("..") != string::npos)
		throw invalid_argument("Invalid path for allowed_data_dir directive: " + allowed_dir + " (cannot contain '..')");
	if (allowed_dir[allowed_dir.length() - 1] != '/')
		allowed_dir.push_back('/');
	if (!(is_directory(allowed_dir)))
		throw invalid_argument("Invalid path for allowed_data_dir directive: " + allowed_dir + " (it must exists)");
	
	location.set_allowed_data_dir(allowed_dir);
}

void	c_parser::parse_methods(c_location & location)
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
			throw invalid_argument("Unexpected value for the method directive in location block: " + get_value());
	}
	if (tmp_methods.empty())
		throw invalid_argument("Directive method can't be empty");
	location.set_methods(tmp_methods);
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void		c_parser::parse_cgi(c_location & location)
{
	string  extension;
	string  path;

	advance_token();
	expected_token_type(TOKEN_VALUE);
	extension = get_value();
	advance_token();
	expected_token_type(TOKEN_VALUE);
	path = get_value();
	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();

	if (extension != ".py" && extension != ".sh" && extension != ".php")
		throw invalid_argument("Invalid extension for the CGI (.py, .sh or .php): " + extension);
	if (path[0] != '/')
		throw invalid_argument("Invalid path for the CGI (must begin with '/'): " + path);
	if (path[path.size() - 1] == '/')
		throw invalid_argument("Invalid path for the CGI: " + path);
	if (!is_executable_file(path))
		throw invalid_argument("No such file or permission denied: " + path);
	location.set_cgi(extension, path);
}

void		c_parser::location_indexes(c_location & location)
{
	advance_token();
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
	   throw invalid_argument("Index directive requires at least one value");

	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void		c_parser::parse_allowed_extensions(c_location & location)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);

	while (is_token_type(TOKEN_VALUE))
	{
		location.add_allowed_extension(_current->value);
		advance_token();
	}
	if (location.get_allowed_extensions().empty())
	   throw invalid_argument("Index directive requires at least one value");

	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void		c_parser::parse_alias(c_location & location)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);
	string  alias = _current->value;
	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();

	if ((alias[0] != '.' || alias[1] != '/') && alias[0] != '.' && alias[0] != '/')
		throw invalid_argument("Invalid path for alias directive): " + alias + " (must begin with './' or '/' )");
	if (alias.find("..") != string::npos)
		throw invalid_argument("Invalid path for alias directive: " + alias + " (cannot contain '..')");
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
		throw invalid_argument("Invalid path for upload_path directive: " + path);
	if (path[path.length() - 1] != '/')
	   throw invalid_argument("Invalid path for upload_path directive: " + path);
	location.set_upload_path(path);
}

void        c_parser::parse_auto_index(c_location & location)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);

	if (_current->value != "ON" && _current->value != "on"
		&& _current->value != "OFF" && _current->value != "off")
		throw invalid_argument("Invalid value for auto_index directive: " + _current->value);
	if (_current->value == "ON" || _current->value == "on")
		location.set_auto_index(true);
	else if (_current->value != "OFF" || _current->value != "off")
		location.set_auto_index(false);

	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void        c_parser::parse_redirect(c_location & location)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);
	string  code = _current->value;
	advance_token();
	expected_token_type(TOKEN_VALUE);
	string  redirect = _current->value;
	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();

	size_t i = 0;
	while (isdigit(code[i]))
		i++;
	if (i != code.length())
		throw invalid_argument("Invalid code for redirect directive: " + code);

	int nb_code = strtol(code.c_str(), NULL, 10);
	if (nb_code != 301 && nb_code != 302 && nb_code != 307 && nb_code != 308)
		throw invalid_argument("Invalid code for redirect directive (it must be 301, 302, 307 or 308): " + code);

	if (redirect.compare(0, 6, "https://") != 0
		&& redirect.compare(0, 7, "http://") != 0
		&& redirect[0] != '/')
		throw invalid_argument("Invalid url for redirect directive (accepted format : 'https://', 'http://' or '/'): " + redirect);

	pair<int, string> redir;
	redir.first = nb_code;
	redir.second = redirect;
	location.set_redirect(redir);
}

/*-----------------------   server : directives   ------------------------------*/

string	c_parser::parse_ip(string const & value)
{
	if (value == "*")
		return ("0.0.0.0");
	if (value.find_first_not_of("0123456789.") != string::npos
		|| count(value.begin(), value.end(), '.') != 3
		|| value.empty())
		throw invalid_argument("Invalid IP adress: " + value);
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
			throw invalid_argument("Invalid IP adress: " + value);
		is_valid_ip = strtol(buf.c_str(), NULL, 10);
		if (errno == ERANGE || is_valid_ip < 0 || is_valid_ip > 255)
			throw invalid_argument("Invalid IP adress: " + value);
		temp.erase(0, pos + 1);
		i++;
	}
	return value;
}

void	c_parser::parse_listen_directive(c_server & server)
{
	long    port = -1;
	string  str_ip;

	advance_token();

	if (get_value().find_first_not_of("0123456789") == string::npos)
	{
		port = strtol(get_value().c_str(), NULL, 10);
		str_ip = "0.0.0.0";
	}
	else
	{
		string  str_port;
		if (count(get_value().begin(), get_value().end(), ':') != 1)
			throw invalid_argument("Invalid port: " + get_value());
		str_port = get_value().substr(get_value().find(':') + 1, get_value().size());
		if (str_port.empty())
			throw invalid_argument("Invalid port: " + get_value());
		if (str_port.find_first_not_of("0123456789") != string::npos)
			throw invalid_argument("Invalid port: " + get_value());
		port = strtol(str_port.c_str(), NULL, 10);
		str_ip = parse_ip(get_value().substr(0, get_value().find(':')));
	}
	if (port == ERANGE || port < 0 || port > 65535)
		throw invalid_argument("Invalid port [0-65535]: " + get_value());

	server.add_port(static_cast<int>(port));
	server.set_ip(str_ip);

	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void	c_parser::parse_index_directive(c_server & server)
{
	vector<string>  index_files;
	advance_token();
	expected_token_type(TOKEN_VALUE);

	while (is_token_type(TOKEN_VALUE))
	{
		index_files.push_back(_current->value);
		advance_token();
	}
	if (index_files.empty())
	   throw invalid_argument("Index directive requires at least one value");

	server.set_indexes(index_files);
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void	c_parser::parse_server_name(c_server & server)
{
	vector<string>  temp_names;
	advance_token();
	expected_token_type(TOKEN_VALUE);

	while (is_token_type(TOKEN_VALUE))
	{
		temp_names.push_back(_current->value);
		advance_token();
	}
	if (temp_names.empty())
		throw invalid_argument("server_name directive requires at least one value");
	server.set_name(temp_names);
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();
}

void		c_parser::parse_root_directive(c_server & server)
{
	advance_token();
	expected_token_type(TOKEN_VALUE);
	string  root = _current->value;
	advance_token();
	expected_token_type(TOKEN_SEMICOLON);
	advance_token();

	if ((root[0] != '.' || root[1] != '/') && root[0] != '.' && root[0] != '/')
		throw invalid_argument("Invalid path for root directive): " + root + " (must begin with './' or '/' )");
	if (root.find("..") != string::npos)
		throw invalid_argument("Invalid path for root directive: " + root + " (cannot contain '..')");
	if (root[root.length() - 1] != '/')
		root.push_back('/');
	if (!(is_directory(root)))
		throw invalid_argument("Invalid path for root directive: " + root + " (it must exists)");
	server.set_root(root);
}

void	c_parser::parse_server_directives(c_server & server)
{
	if (is_token_value("index"))
		parse_index_directive(server);
	else if (is_token_value("listen"))
		parse_listen_directive(server);
	else if (is_token_value("server_name"))
		parse_server_name(server);
	else if (is_token_value("client_max_body_size"))
		parse_body_size(server);
	else if (is_token_value("error_page"))
		parse_error_page(server);
	else if (is_token_value("root"))
		parse_root_directive(server);
	else
		throw invalid_argument("Unexpected token in server block (not a valid directive): " + _current->value);
}

/*-------------------   parse server block ---------------------*/

c_server	c_parser::parse_server_block()
{
	c_server    server;
	bool        has_location = false;

	advance_token();
	expected_token_type(TOKEN_LBRACE);
	advance_token();
	while (!is_token_type(TOKEN_RBRACE) && !is_at_end())
	{
		if (is_token_type(TOKEN_DIRECTIVE_KEYWORD))
		{
			if (has_location)
				throw invalid_argument("Server directive is forbidden after location block");
			parse_server_directives(server);
		}
		else if (is_token_type(TOKEN_BLOC_KEYWORD) && is_token_value("location"))
		{
			parse_location_block(server);
			has_location = true;
		}
		else
		{
			throw invalid_argument("Unexpected token in server block: " + _current->value);
		}
	}
	expected_token_type(TOKEN_RBRACE);
	advance_token();
	if (server.get_ports().empty())
		throw invalid_argument("The block server must define at least one port");
	if (server.get_root().empty())
		throw invalid_argument("The block server must define a root");
	return server;

}

/*-------------------   loop method ---------------------*/

vector<c_server>    c_parser::parse_config()
{
	vector<c_server> servers;

	while (!is_at_end())
	{
		s_token token = current_token();
		if (is_token_value("server") && is_token_type(TOKEN_BLOC_KEYWORD))
		{
			c_server server = parse_server_block();
			servers.push_back(server);
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
		return vector<c_server>();
	}
}

/*-----------------------   utils -----------------------*/

size_t	c_parser::convert_to_octet(string const & str, string const & suffix, size_t const i) const
{
	size_t limit = 0;
	string number_part;

	if (suffix.empty())
		number_part = str;
	else
		number_part = str.substr(0, i);
	errno = 0;
	for (size_t j = 0; j < number_part.length(); ++j)
	{
		if (!isdigit(number_part[j]))
			throw invalid_argument("Invalid argument for max_body_size (invalid character): " + str);
		if (limit > (MY_SIZE_MAX - (number_part[j] - '0')) / 10)
			throw invalid_argument("Invalid argument for max_body_size (number too large): " + str);
		limit = limit * 10 + (number_part[j] - '0');
	}
	if (errno == ERANGE)
		throw invalid_argument("Invalid argument for max_body_size (unexpected conversion of the number): " + str);
	
	size_t multiplier = 1;
	if (!suffix.empty())
	{
		
		if (suffix == "k" || suffix == "K")
			multiplier = 1024;
		else if (suffix == "m" || suffix == "M")
			multiplier = 1024 * 1024;
		else if (suffix == "g" || suffix == "G")
			multiplier = 1024 * 1024 * 1024;
		else
			throw invalid_argument("invalid argument for max_body_size (unknown sufix): " + str);
		if (limit > MY_SIZE_MAX / multiplier)
			throw invalid_argument("invalid argument for max_body_size (result would overflow):" + str);
		limit *= multiplier;
	}
	if (limit > MAX_BODY_SIZE)
		throw invalid_argument("max_body_size exeeds maximum allowed value");
	
	return limit;
}

/*-----------------   error handling -------------------*/

void	c_parser::throw_error(string const & first, string const & second, string const & value)
{
	throw invalid_argument(first + second + value);
}
