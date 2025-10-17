#include "cgi.hpp"

c_cgi::c_cgi(c_server& server, int client_fd) : 
_server(server), _client_fd(client_fd)
{
	this->_loc = NULL;
	this->_script_name = "";
	this->_path_info = "";
	this->_translated_path ="";
	this->_interpreter = "";
	this->_relative_argv = "";
	this->_relative_script_name = "";
	this->_finished = false;
	this->_content_length = 0;
	this->_body_size = 0;
	this->_pid = 0;
	this->_headers_parsed = false;
	this->_pipe_in = 0;
	this->_pipe_out = 0;
	this->_write_buffer.clear();
	this->_read_buffer.clear();
	this->_bytes_written = 0;
	this->_map_env_vars.clear();
	this->_vec_env_vars.clear();
	this->_request_fully_sent_to_cgi = false;
	this->_body_to_send.clear();
	this->_body_sent = 0;
	this->_body_fully_sent = false;
	this->_start_time = time(NULL);
}

c_cgi::c_cgi(const c_cgi& other): _server(other._server)
{
	this->_client_fd = other._client_fd;
	this->_pipe_in = other._pipe_in;
	this->_pipe_out = other._pipe_out;
	this->_pid = other._pid;
	this->_write_buffer = other._write_buffer;
	this->_read_buffer = other._read_buffer;
	this->_bytes_written = other._bytes_written;
	this->_finished = other._finished;
	this->_map_env_vars = other._map_env_vars;
	this->_vec_env_vars = other._vec_env_vars;
	this->_content_length = other._content_length;
	this->_body_size = other._body_size;
	this->_status_code = other._status_code;
	this->_loc = other._loc;
	this->_script_name = other._script_name;
	this->_path_info = other._path_info;
	this->_translated_path = other._translated_path;
	this->_interpreter = other._interpreter;
	this->_relative_argv = other._relative_argv;
	this->_relative_script_name = other._relative_script_name;
	this->_request_fully_sent_to_cgi = other._request_fully_sent_to_cgi;
	this->_body_to_send = other._body_to_send;
	this->_body_sent = other._body_sent;
	this->_body_fully_sent = other._body_fully_sent;
	this->_start_time = other._start_time;
}

c_cgi const& c_cgi::operator=(const c_cgi& rhs)
{
	if (this != &rhs)
	{
		this->_server = rhs._server;
		this->_client_fd = rhs._client_fd;
		this->_pipe_in = rhs._pipe_in;
		this->_pipe_out = rhs._pipe_out;
		this->_pid = rhs._pid;
		this->_write_buffer = rhs._write_buffer;
		this->_read_buffer = rhs._read_buffer;
		this->_bytes_written = rhs._bytes_written;
		this->_finished = rhs._finished;
		this->_content_length = rhs._content_length;
		this->_headers_parsed = rhs._headers_parsed;
		this->_map_env_vars = rhs._map_env_vars;
		this->_vec_env_vars = rhs._vec_env_vars;
		this->_body_size = rhs._body_size;
		this->_status_code = rhs._status_code;
		this->_loc = rhs._loc;
		this->_script_name = rhs._script_name;
		this->_path_info = rhs._path_info;
		this->_translated_path = rhs._translated_path;
		this->_interpreter = rhs._interpreter;
		this->_relative_argv = rhs._relative_argv;
		this->_relative_script_name = rhs._relative_script_name;
		this->_request_fully_sent_to_cgi = rhs._request_fully_sent_to_cgi;
		this->_body_to_send = rhs._body_to_send;
		this->_body_sent = rhs._body_sent;
		this->_body_fully_sent = rhs._body_fully_sent;
		this->_start_time = rhs._start_time;
	}
	return (*this);
}

c_cgi::~c_cgi()
{
	//cout << "DESCTRUCTOR DE CGI" << endl;
	if (this->get_pipe_in() > 0)
		close(this->get_pipe_in());
	if (this->get_pipe_out() > 0)
		close(this->get_pipe_out());
}

void    c_cgi::clear_context()
{
	this->_loc = NULL;
	this->_script_name = "";
	this->_path_info = "";
	this->_translated_path ="";
	this->_interpreter = "";
	this->_relative_argv = "";
	this->_relative_script_name = "";
	this->_finished = false;
	this->_content_length = 0;
	this->_body_size = 0;
	this->_headers_parsed = false;
	this->_pipe_in = 0;
	this->_pipe_out = 0;
	this->_write_buffer.clear();
	this->_read_buffer.clear();
	this->_bytes_written = 0;
	this->_map_env_vars.clear();
	this->_vec_env_vars.clear();
	this->_request_fully_sent_to_cgi = false;
}

void    c_cgi::append_read_buffer(const char* buffer, ssize_t bytes)
{
	if (!buffer || bytes == 0)
		return ;
	if (_read_buffer.size() + bytes > 10 * 1024 * 1024) 
	{
		_server.log_message("[ERROR] CGI output too large, killing process");
		kill(this->get_pid(), SIGKILL);
		return;
	}
	this->_read_buffer.append(buffer, bytes);
}

void    c_cgi::consume_read_buffer(size_t n) 
{
	if (n >= _read_buffer.size())
		_read_buffer.clear();
	else
		_read_buffer.erase(0, n);
}

map<string, c_location>::const_iterator   find_location(const string &path, map<string, c_location>& map_location)
{
	map<string, c_location>::const_iterator best_match = map_location.end();
	size_t  max_len = 0;

	for (map<string, c_location>::const_iterator it = map_location.begin(); it != map_location.end(); it++)
	{
		const string& loc_path = it->first;
		if (path.compare(0, loc_path.size(), loc_path) == 0 && loc_path.size() > max_len)
		{
			best_match = it;
			max_len = loc_path.size();
		}
	}
	return (best_match);
}

string  find_script_extension(const string& target)
{
	string empty_string = "";
	size_t dot_position = target.find('.');

	if (dot_position == 0)
		dot_position = target.find('.');
	if (dot_position != string::npos)
	{
		string tmp_extension = target.substr(dot_position);
		size_t slash_position = tmp_extension.find("/");
		string extension;
		if (slash_position != string::npos)
		   extension = tmp_extension.substr(0, slash_position);
		else
			extension = tmp_extension;
		return (extension);
	}
	return (empty_string);
}

size_t c_cgi::identify_script_type(const string& path)
{
	size_t pos_script = string::npos;
	string  script_type;

	pos_script = path.find(".py");
	if (pos_script == string::npos)
	{
		pos_script = path.find(".sh");
		if (pos_script == string::npos)
		{
			pos_script= path.find(".php");
			if (pos_script == string::npos)
			{
				_server.log_message("[ERROR] Unknown script type");
				this->set_exit_status(404);
				return (string::npos);
			}
			 this->_script_name = path.substr(0, pos_script + 4);
			 this->_path_info = path.substr(pos_script + 4);
			return (pos_script + 4);
		}
	}
	this->_script_name = path.substr(0, pos_script + 3);
	this->_path_info = path.substr(pos_script + 3);
	return (pos_script + 3);
}

int    c_cgi::resolve_cgi_paths(const c_location &loc, string const& script_filename)
{
	size_t ext_pos = identify_script_type(script_filename);
	if (ext_pos == string::npos)
	{
		return(1);
	}
	string  root = loc.get_alias();
	string  url_key = loc.get_url_key();

	string  relative_path = this->_script_name.substr(url_key.size());
	this->_script_filename = root + relative_path;


	char resolved_path[PATH_MAX];
	if (!this->_script_filename.empty() && !realpath(this->_script_filename.c_str(), resolved_path))
	{
		this->set_exit_status(403);
		return (1);
	}

	this->_relative_script_name = this->_script_name.substr(url_key.size());
	if (!this->_path_info.empty())
	{
		this->_relative_argv = "./www" + this->_path_info;
		// changer en loc.get_root()
		this->_translated_path = "./www" + this->_path_info;
	}
	return(0);
}

bool	c_cgi::is_argv_in_allowed_directory(const string& argv, const string& allowed_data_dir)
{
	char	resolved_path[PATH_MAX];
	char	resolved_dir[PATH_MAX];

	if (!realpath(argv.c_str(), resolved_path))
		return (false);

	if (!realpath(allowed_data_dir.c_str(), resolved_dir))
		return (false);

	string	path = resolved_path;
	string	directory = resolved_dir;

	if (path.rfind(directory, 0) == 0)
		return (true);

	return (false);
}

int    c_cgi::init_cgi(const c_request &request, const c_location &loc, string target)
{
	
	this->_status_code = request.get_status_code();
	this->_loc = &loc;
	char cwd[1024];

	if (getcwd(cwd, sizeof(cwd)) == NULL) 
	{
		_server.log_message("[ERROR] getcwd failed");
		return (1);
	}
	std::string old_dir = cwd;
 
	string extension = find_script_extension(target);

	if (resolve_cgi_paths(loc, request.get_target()))
		return (1);

	string allowed_data_dir = "./www/data/"; // à retirer en allant checher directement dans la location
	if (!this->_relative_argv.empty() && !is_argv_in_allowed_directory(this->_relative_argv, allowed_data_dir))
	{
		if (chdir("www/cgi-bin/") == 0)
		{
			ifstream file_checker(this->_relative_argv.c_str());
			if (!file_checker.is_open())
			{
				if (chdir(old_dir.c_str()) != 0)
					_server.log_message("[ERROR] chdir failed");
				this->_status_code = 404;
				return (1);
			}
			else
			{
				file_checker.close();
				if (chdir(old_dir.c_str()) != 0)
				{
					_server.log_message("[ERROR] chdir failed");
					return (1);
				}
			}
		}
		if (this->_relative_argv.find("data") == string::npos)
		{
			if (chdir(old_dir.c_str()) != 0)
				_server.log_message("[ERROR] chdir failed");
			this->_status_code = 403;
			clear_context();
			_server.log_message("[WARNING] CGI's argument is outside data file");
			return (1);
		}
	}

	if (extension.size() > 0)
		this->_interpreter = loc.extract_interpreter(extension);
	if (this->_interpreter.empty())
		return (1);

	set_environment(request);
	return (0);
}

void  c_cgi::vectorize_env()
{
	map<string, string> map_copy = this->_map_env_vars;

	for (map<string, string>::const_iterator it = map_copy.begin(); it != map_copy.end(); it++)
		this->_vec_env_vars.push_back(it->first + "=" + it->second);
}

void    c_cgi::set_environment(const c_request &request)
{
    this->_socket_fd = request.get_socket_fd();
    this->_map_env_vars["REQUEST_METHOD"] = request.get_method();
    this->_map_env_vars["SCRIPT_NAME"] = this->_script_name;
    this->_map_env_vars["PATH_INFO"] = this->_path_info;
    this->_map_env_vars["TRANSLATED_PATH"] = this->_translated_path;
    this->_map_env_vars["SCRIPT_FILENAME"] = this->_script_filename;
	this->_map_env_vars["CONTENT_LENGTH"] = int_to_string(request.get_body().size());
    this->_map_env_vars["CONTENT_TYPE"] = request.get_header_value("Content-Type");
    this->_map_env_vars["QUERY_STRING"] = request.get_query();
    this->_map_env_vars["SERVER_PROTOCOL"] = request.get_version();
    this->_map_env_vars["GATEWAY_INTERFACE"] = "CGI/1.1";
    this->_map_env_vars["REMOTE_ADDR"] = request.get_ip_client();
    this->_map_env_vars["REDIRECT_STATUS"] = int_to_string(this->_status_code);
    this->_map_env_vars["HTTP_ACCEPT"] = request.get_header_value("Accept");
    this->_map_env_vars["HTTP_USER_AGENT"] = request.get_header_value("User-Agent");
    this->_map_env_vars["HTTP_ACCEPT_LANGUAGE"] = request.get_header_value("Accept-Language");
    this->_map_env_vars["HTTP_COOKIE"] = request.get_header_value("Cookie");
    this->_map_env_vars["HTTP_REFERER"] = request.get_header_value("Referer");
    this->_map_env_vars["HTTP_HOST"] = request.get_header_value("Host");

	this->vectorize_env();
}

string make_absolute(const string &path)
{
	char resolved[1000];

	if (realpath(path.c_str(), resolved))
		return (string(resolved));

	return (path);
}

int c_cgi::launch_cgi(vector<char>& body)
{
	int server_to_cgi[2];
	int cgi_to_server[2];

	if (pipe(server_to_cgi) < 0 || pipe(cgi_to_server) < 0)
	{
		_server.log_message("[ERROR] pipe error");
		return (500);
	}

	this->_pipe_in = server_to_cgi[1];
	this->_pipe_out = cgi_to_server[0];
	this->_body_to_send = body;
	this->_body_sent = 0;
	this->_body_fully_sent = false;
	this->_start_time = time(NULL);
	this->_read_buffer.clear();
	this->_bytes_written = 0;

	int flags_out = fcntl(this->_pipe_out, F_GETFL, 0);
	int flags_in = fcntl(this->_pipe_in, F_GETFL, 0);

	if (flags_out == -1 || flags_in == -1) 
	{
		_server.log_message("[ERROR] flags fcntl failed");
		return (500);
	}

	if (fcntl(this->_pipe_out, F_SETFL, flags_out | O_NONBLOCK) == -1 || fcntl(this->_pipe_in, F_SETFL, flags_in| O_NONBLOCK) == -1) 
	{
		_server.log_message("[ERROR] flags fcntl failed");
		return (500);
	}

	this->_pid = fork();
	if (this->_pid < 0)
	{
		_server.log_message("[ERROR] fork failed");
		return (500);
	}

	if (this->_pid == 0)
	{
		dup2(server_to_cgi[0], STDIN_FILENO);
		close(server_to_cgi[0]);
		close(this->_pipe_in);
		dup2(cgi_to_server[1], STDOUT_FILENO);
		close(cgi_to_server[1]);
		close(this->_pipe_out);
	 
		vector<char*> envp;
		for (size_t i = 0; i < this->_vec_env_vars.size(); i++)
			envp.push_back(const_cast<char *>(this->_vec_env_vars[i].c_str()));
		envp.push_back(NULL);

		
		if (!this->_relative_argv.empty())
		{
			if (chdir(_loc->get_alias().c_str()) != 0) 
				exit(1);

			this->_relative_argv = "../" + this->_relative_argv.substr(5); // à adapter en fonction du root

			char *argv[4];
			argv[0] = const_cast<char*>(this->_interpreter.c_str());
			argv[1] = const_cast<char*>(this->_relative_script_name.c_str());
			argv[2] = const_cast<char*>(this->_relative_argv.c_str());
			argv[3] = NULL;
			execve(this->_interpreter.c_str(), argv, &envp[0]);
		}
		else
		{
			char *argv[3];
			argv[0] = const_cast<char*>(this->_interpreter.c_str());
			argv[1] = const_cast<char*>(this->_script_filename.c_str());
			argv[2] = NULL;
			execve(this->_interpreter.c_str(), argv, &envp[0]);
		}
		exit(1);
	}

	close(server_to_cgi[0]);
	close(cgi_to_server[1]);

	_server.add_fd(this->_pipe_in, POLLOUT);
	_server.add_fd(this->_pipe_out, POLLIN);

	return (200);
}

