#include "server.hpp"
#include "clients.hpp"

size_t	c_server::extract_content_length(string headers)
{
	string key = "Content-Length:";
	size_t pos = headers.find(key);

	if (pos == string::npos)
	{
		return (0);
	}
	pos += key.length();

	size_t	end = headers.find("\r\n", pos);
	if (end == string::npos)
		end = headers.length();

	string value = headers.substr(pos, end - pos);
	value = ft_trim(value);
	for (size_t i = 0; i < value.size(); i++)
	{
		if (!isdigit(value[i]))
			return (0);
	}

	size_t content_length = strtol(value.c_str(), 0, 10);

	return (content_length);
}


void	c_server::transfer_by_bytes(c_cgi *cgi, const string& buffer, size_t bytes)
{
	c_client *client = find_client(cgi->get_client_fd());
	
	if (client)
	{
		client->get_write_buffer().append(buffer, 0, bytes);
		client->append_response_body_size(buffer.size());
	}
	if (client->get_response_body_size() == cgi->get_content_length() && cgi->get_content_length() > 0)
	{
		cgi->set_finished(true);
	}
}

void	c_server::transfer_with_chunks(const char *buffer, size_t bytes, c_cgi *cgi)
{
	c_client *client = find_client(cgi->get_client_fd());
	if (!client)
		return ;

	std::string chunk;

	chunk = int_to_hex(bytes) + "\r\n";
	chunk.append(buffer, bytes);
	chunk.append("\r\n");

	client->get_write_buffer().append(chunk);
}

void	c_server::fill_cgi_response_headers(string headers, c_cgi *cgi)
{
	c_client *client = find_client(cgi->get_client_fd());

	if (client)
	{
		client->get_write_buffer().append("HTTP/1.1 200 OK\r\n");
		if (!headers.empty())
		{
			cgi->set_content_length(extract_content_length(headers));
			client->get_write_buffer().append(headers + "\r\n");
		}
		if (cgi->get_read_buffer().find("Content-Type") == string::npos)
			client->get_write_buffer().append("Content-Type: text/plain\r\n");
		if (cgi->get_content_length() == 0)
			client->get_write_buffer().append("Transfer-Encoding: chunked\r\n");
		client->get_write_buffer().append("\r\n");
		cgi->set_headers_parsed(true);
	}
}

void	c_server::handle_fully_sent_response(c_client *client)
{
	int duration = client->get_last_modified() - client->get_creation_time();
	log_message("[INFO] ✅ RESPONSE FULLY SENT TO CLIENT " 
				+ int_to_string(client->get_fd()) + " IN " + int_to_string(duration) + "s with status_code of " + int_to_string(client->get_status_code()));
	log_access(client);
	log_message("[DEBUG] Client " + int_to_string(client->get_fd()) 
				+ " can send a new request : POLLIN");
	client->set_last_modified();
	// client->set_creation_time();
	client->set_bytes_written(0);
	client->get_request()->init_request();
	client->get_response()->init_response();

	client->clear_read_buffer();
	client->clear_write_buffer();
	client->set_response_complete(false);
	client->set_state(READING);
	return ;
}

void	c_server::fill_cgi_response_body(const char *buffer, size_t bytes, c_cgi *cgi)
{
	if (cgi->get_content_length() == 0)
		transfer_with_chunks(buffer, bytes, cgi);
	else
		transfer_by_bytes(cgi, buffer, bytes);
}