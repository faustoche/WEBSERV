#pragma once

#include "server.hpp"
#include <sys/wait.h>

class   c_response;
class   c_request;
class   c_location;

class c_cgi
{
    public:
        c_cgi(c_server& server, int client_fd);
        c_cgi const& operator=(const c_cgi& rhs);
        c_cgi (const c_cgi& other);
        ~c_cgi();


        int             init_cgi(const c_request &request, const c_location &loc);
        string          launch_cgi(const string &body);
        int             resolve_cgi_paths(const c_location &loc, const string& filename);
        void            set_environment(const c_request &request);
        void            set_script_filename(const string& filename) { this->_script_filename = filename; };
        const string&   get_script_filename() const { return this->_script_filename; };
        void            get_header_from_cgi(c_response &response, const string& content_cgi);
        const string&   get_interpreter() const { return _interpreter; };
        const int&      get_client_fd() const { return _client_fd; };
        const int&      get_pipe_in() const { return _pipe_in; };
        const int&      get_pipe_out() const { return _pipe_out; };
        const pid_t&    get_pid() const { return _pid; };
        const string&   get_write_buffer() const { return _write_buffer; };
        const string&   get_read_buffer() const { return _read_buffer; };
        const size_t&   get_bytes_written() const { return _bytes_written; };
        const size_t&   get_content_length() const { return _content_length; };
        bool            is_finished() { return _finished; };
        bool            headers_parsed() { return _headers_parsed; };
        void            set_finished(bool state) { _finished = state; };
        void            set_exit_status(int status) { _status_code = status; };
        void            set_headers_parsed(bool state) { _headers_parsed = state; };
        void            consume_read_buffer(size_t n);
        void            set_content_length(size_t bytes) { _content_length = bytes; };

        void            add_bytes_written(ssize_t bytes) { _bytes_written += bytes; };
        void            append_read_buffer(const char* buffer, ssize_t bytes);
        int             parse_headers(c_response &response, string& headers);
        bool            is_valid_header_value(string& key, const string& value);
        void            vectorize_env();
        void            close_cgi();
        size_t          identify_script_type(const string& path);

    private:
        c_server&           _server;
        int                 _client_fd;
        int                 _pipe_in;
        int                 _pipe_out;
        pid_t               _pid;
        string              _write_buffer; //body a envoyer
        string              _read_buffer; // reponse CGI accumulee
        size_t              _bytes_written;
        size_t              _content_length;
        bool                _finished;
        bool                _headers_parsed;

        map<string, string> _map_env_vars;
        vector<string>      _vec_env_vars;
        int                 _socket_fd; // pas necessaire une fois connecte a la classe reponse
        int                 _status_code;
        const c_location*   _loc;
        string              _script_name;
        string              _path_info;
        string              _translated_path;
        string              _script_filename;
        string              _interpreter;
};