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

string	get_unique_filename(const string &directory, string &filename)
{
	string full_path = directory + filename;
	if (!file_exists(full_path))
		return filename;
	
	size_t dot_pos = filename.find_last_of('.');
	string name = filename.substr(0, dot_pos);
	string extension = filename.substr(dot_pos);

	int counter = 1;
	string new_filename;

	while (true)
	{
		ostringstream oss;
		oss << counter;
		new_filename = name + "_" + oss.str() + extension;
		full_path = directory + new_filename;

		if (!file_exists(full_path))
			return new_filename;
		counter++;
		if (counter > 1000)
		{
			ostringstream oss_t;
			oss_t << time(0);
			return name + "_" + oss_t.str() + extension;
		}
	}
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