#pragma once

#include "server.hpp"
#include <sys/wait.h>

class	c_response;
class	c_request;
class	c_location;

class c_cgi
{
	public:
		c_cgi(c_server& server, int client_fd);
		c_cgi const& operator=(const c_cgi& rhs);
		c_cgi (const c_cgi& other);
		~c_cgi();

		int				init_cgi(const c_request &request, const c_location &loc, string file_path);
		int				launch_cgi(const string &body);
		int				resolve_cgi_paths(const c_location &loc, const string& filename);
		void			set_environment(const c_request &request);
		void			set_script_filename(const string& filename) { this->_script_filename = filename; };
		const string&	get_script_filename() const { return this->_script_filename; };
		void			get_header_from_cgi(c_response &response, const string& content_cgi);
		const string&	get_interpreter() const { return _interpreter; };
		const string&	get_relative_script_name() const { return _relative_script_name; };
		const string&	get_relative_argv() const { return _relative_argv; };
		const int&		get_client_fd() const { return _client_fd; };
		const int&		get_pipe_in() const { return _pipe_in; };
		const int&		get_pipe_out() const { return _pipe_out; };
		const pid_t&	get_pid() const { return _pid; };
		const string&	get_write_buffer() const { return _write_buffer; };
		const string&	get_read_buffer() const { return _read_buffer; };
		// const size_t&   get_bytes_written() const { return _bytes_written; };
		const size_t&	get_content_length() const { return _content_length; };
		const int&		get_status_code() const { return _status_code; };
		bool			is_finished() { return _finished; };
		bool			headers_parsed() { return _headers_parsed; };
		void			set_finished(bool state) { _finished = state; };
		void			set_exit_status(int status) { _status_code = status; };
		void			set_headers_parsed(bool state) { _headers_parsed = state; };
		void			consume_read_buffer(size_t n);
		void			set_content_length(size_t bytes) { _content_length = bytes; };
		// void            set_body_size(size_t bytes) { _body_size = bytes; };
		const size_t&	get_body_size() { return _body_size; };
		const string&	get_body_to_send() const { return _body_to_send; };
		const size_t&	get_body_sent() const { return _body_sent; };
		// void            mark_request_fully_sent() { _request_fully_sent_to_cgi = true; };
		void			mark_stdin_closed() { _pipe_in = -1; };
		void			mark_stdout_closed() { _pipe_out = -1; };
		void			set_body_fully_sent_to_cgi() { _body_fully_sent = true; };

		// void            add_bytes_written(ssize_t bytes) { _bytes_written += bytes; };
		void			add_body_sent(ssize_t bytes) { _body_sent += bytes; };
		void			append_read_buffer(const char* buffer, ssize_t bytes);
		int				parse_headers(c_response &response, string& headers);
		bool			is_valid_header_value(string& key, const string& value);
		// bool            is_request_fully_sent_to_cgi() { return _request_fully_sent_to_cgi; };
		bool			is_body_fully_sent_to_cgi() { return _body_fully_sent; };
		void			vectorize_env();
		void			clear_context();
		void			close_cgi();
		size_t			identify_script_type(const string& path);

	private:
		c_server&			_server;
		int					_client_fd;
		int					_pipe_in;
		int					_pipe_out;
		pid_t				_pid;
		string				_body_to_send;
		size_t				_body_sent;
		string				_write_buffer; //body a envoyer
		string				_read_buffer; // reponse CGI accumulee
		size_t				_bytes_written;
		size_t				_content_length;
		size_t				_body_size;
		bool				_body_fully_sent;
		bool				_finished;
		bool				_headers_parsed;
		bool				_request_fully_sent_to_cgi;
		time_t				_start_time;

		map<string, string>	_map_env_vars;
		vector<string>		_vec_env_vars;
		int					_socket_fd; // pas necessaire une fois connecte a la classe reponse
		int					_status_code;
		const c_location*	_loc;
		string				_script_name;
		string				_relative_script_name;
		string				_relative_argv;
		string				_path_info;
		string				_translated_path;
		string				_script_filename;
		string				_interpreter;
};