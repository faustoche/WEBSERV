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


void	c_server::transfer_by_bytes(c_cgi *cgi, const string& buffer)
{
	c_client *client = find_client(cgi->get_client_fd());
	if (client)
	{
	    client->get_write_buffer().append(buffer);
		client->append_response_body_size(buffer.size());

		if (client->get_response_body_size() == cgi->get_content_length() && cgi->get_content_length() > 0)
			cgi->set_finished(true);
	}
}

void	c_server::transfer_with_chunks(c_cgi *cgi, const string& buffer)
{
	c_client *client = find_client(cgi->get_client_fd());
	if (client)
	{
		std::string chunk;
    	size_t chunk_size = buffer.size();

    	// Construire un chunk avec la taille en hex
    	chunk = int_to_hex(chunk_size) + "\r\n" +
    	        buffer + "\r\n";

    	client->get_write_buffer().append(chunk);
	}
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

void	c_server::fill_cgi_response_body(string body_part, c_cgi *cgi)
{
	if (cgi->get_content_length() == 0)
	{
		// Si un '\0' final a ete ajoute par la lecture, on l'enleve
		if (!body_part.empty() && body_part[body_part.size() - 1] == '\0')
			body_part.erase(body_part.size() - 1);
		transfer_with_chunks(cgi, body_part);
	}
	else
		transfer_by_bytes(cgi, body_part);
}