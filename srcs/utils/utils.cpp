#include "server.hpp"

std::string	int_to_string(int n)
{
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

std::string int_to_hex(size_t value) 
{
	std::stringstream ss;
	ss << hex << value;
	return ss.str();
}

bool    is_valid_header_name(const string& key_name)
{
	const string allowed_special_chars = "!#$%&'*+-.^_`|~";

	if (key_name.empty())
	{
		cerr << "(Request) Error: empty header name." << endl;
		return (false);
	}
	for (size_t i = 0; i < key_name.length(); i++)
	{
		if (!isalnum(key_name[i]) && allowed_special_chars.find(key_name[i]) == string::npos)
		{
			cerr << "(Request) Error: Invalid char in header name: " << key_name << endl;
			return (false);
		}
	}
	return (true);
}

string  ft_trim(const string& str)
{
	size_t start = 0;
	size_t end = str.length();

	while (start < end && (str[start] == ' ' || str[start] == '\t'))
		start++;

	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
		end--;
	
	return (str.substr(start, end - start));
}

bool        is_readable_file(const string & path)
{
	return access(path.c_str(), R_OK) == 0;
}

bool        is_existing_file(const string & path)
{
	return (access(path.c_str(), F_OK) == 0);
}

bool        is_executable_file(const string & path)
{
	return access(path.c_str(), X_OK) == 0;
}

string  trim(const string &str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == string::npos)
		return "";
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

string  trim_underscore(const string &str)
{
	size_t start = str.find_first_not_of("_");
	if (start == string::npos)
		return "";
	size_t end = str.find_last_not_of("_");
	return str.substr(start, end - start + 1);
}

bool	file_exists(const std::string &path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

bool	directory_exists(const string &path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool	create_directory(const string &path)
{
	return (mkdir(path.c_str(), 0755) == 0);
}

bool	is_directory(const string& path)
{
	struct stat path_stat;

	if (stat(path.c_str(), &path_stat) != 0)
		return (false);
	return (S_ISDIR(path_stat.st_mode));
}

bool	is_regular_file(const string& path)
{
	struct stat path_stat;

	if (stat(path.c_str(), &path_stat) != 0)
		return (false);
	return (S_ISREG(path_stat.st_mode));
}


string url_decode(const string &str)
{
	string ret;
	for (string::size_type i = 0; i < str.length(); ++i)
	{
		if (str[i] == '%')
		{
			if (i + 2 < str.length())
			{
				char hex[3];
				hex[0] = str[i+1];
				hex[1] = str[i+2];
				hex[2] = '\0';
				char decoded = static_cast<char>(strtol(hex, NULL, 16));
				ret += decoded;
				i += 2;
			}
		}
		else if (str[i] == '+')
			ret += ' ';
		else
			ret += str[i];
	}
	return (ret);
}

string escape_html(const string &text)
{
	string escaped;
	for (string::size_type i = 0; i < text.length(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(text[i]);
		switch (c)
		{
			case '&':  escaped += "&amp;";  break;
			case '<':  escaped += "&lt;";   break;
			case '>':  escaped += "&gt;";   break;
			case '"':  escaped += "&quot;"; break;
			case '\'': escaped += "&#39;";  break;
			default:
				if (c < 32 || c > 126)
				{
					char buf[8];
					sprintf(buf, "\\x%02X", c);
					escaped += buf;
				}
				else
					escaped += text[i];
		}
	}
	return (escaped);
}

string escape_html_attr(const string &text)
{
	string escaped;
	for (string::size_type i = 0; i < text.length(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(text[i]);
		switch (c)
		{
			case '&':  escaped += "&amp;";  break;
			case '<':  escaped += "&lt;";   break;
			case '>':  escaped += "&gt;";   break;
			case '"':  escaped += "&quot;"; break;
			case '\'': escaped += "&#39;";  break;
			default:
				escaped += text[i];
		}
	}
	return (escaped);
}

/* fonciton qui verifie si on va pas join deux // cote a cote 
./www/ + /data/ → ./www/data/
./www + data/ → ./www/data/
./www/ + data/ → ./www/data/
./www + /data/ → ./www/data/
*/
string	join_path(const string &base, const string &path)
{
	if (base.empty())
		return path;
	if (path.empty())
		return base;
	
	bool base_ends_slash = (base[base.length() - 1] == '/');
	bool path_starts_slash = (path[0] == '/');

	if (base_ends_slash && path_starts_slash)
		return base + path.substr(1);
	else if (!base_ends_slash && !path_starts_slash)
		return base + '/' + path;
	else
		return base + path;
}