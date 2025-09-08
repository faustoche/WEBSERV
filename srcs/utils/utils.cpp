#include "server.hpp"

std::string	int_to_string(int n){
	std::ostringstream oss;
	oss << n;
	return (oss.str());
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