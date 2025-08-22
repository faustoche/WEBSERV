#pragma once

/*-----------  INCLUDES -----------*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;

/*-----------  CLASS -----------*/

class c_response
{
private:
	string _response;

public:
	void define_response_content();
	const string &get_response() const;

private:
	/* Méthodes privées pour la construction des réponses*/
	void build_success_response(string &file_path, string version);
	string load_file_content(string &file_path);
	// echec
	// get content type
	// load file content
};