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


string  sanitize_filename(const string &filename)
{
    string name;
    string extension = extract_extension(filename, name);

    if (extension.empty())
        return "";
    if (name.empty())
        name = "file";

    string clean_name;
    for(size_t i = 0; i < name.size(); i++)
    {
        char c = name[i];
        if (isalnum(static_cast<unsigned char>(c)))
            clean_name += c;
        else if (c == '_' || c == '-' || c == ' ')
            clean_name += '_';
        else
            clean_name += '_';
    }
    while (clean_name.find("__") != string::npos)
        clean_name.replace(clean_name.find("__"), 2, "_");
    while (!clean_name.empty() && clean_name[clean_name.size() - 1] == '_')
        clean_name.erase(clean_name.size() - 1);
    if (clean_name.size() > 200)
        clean_name = clean_name.substr(0, 200);
    clean_name = trim_underscore(clean_name);
    if (!extension.empty())
        return clean_name += "." + extension;
    else
        return clean_name;
}


string  extract_extension(const string &filename, string &name)
{
    size_t point_pos = filename.find_last_of(".");
    if (point_pos == string::npos)
        return "";
    if (point_pos == 0)
        return "";
    string extension = filename.substr(point_pos + 1);
    name = filename.substr(0, point_pos);
    if (extension != "jpg" && extension != "jpeg" && extension != "png" && extension != "gif"
        && extension != "pdf" && extension != "txt")
    {
        cout << "Error: extension not allowded (." << extension << ")" << endl;
        // revoir comment gerer l'erreur (peut etre juste ecrire que le telechargement n'a pas reussi)
        // est-ce que si l'extension ets vide on en met une par defaut ?
        return "";
    }
    return extension;
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