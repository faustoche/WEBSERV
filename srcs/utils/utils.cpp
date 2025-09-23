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

bool        is_readable_file(const std::string & path)
{
    return access(path.c_str(), R_OK) == 0;
}

bool        is_executable_file(const std::string & path)
{
    return access(path.c_str(), X_OK) == 0;
}

// fonction qui ne fonctionne qu'avec l'index du server a priori
string      get_valid_index(string const & root, vector<string> const & indexes)
{
    for (vector<string>::const_iterator it = indexes.begin(); it != indexes.end(); it++)
    {
        string  full_path = root + "/www/" + *it;
        if (is_readable_file(full_path))
            return *it;
    }
    return "";
}

// string get_valid_index(vector<string> const & indexes) 
// { 
//     string valid_file; 
//     vector<string>::const_iterator it = indexes.begin(); 
//     while (it != indexes.end()) 
//     { 
//         if (is_executable_file(*it)) 
//         { 
//             valid_file = *it; 
//             break ;
//         } 
//         it++; 
//     } 
//     if (valid_file.empty()) 
//         return ""; 
//     return valid_file; 
// }