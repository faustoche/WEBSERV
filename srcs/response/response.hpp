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
	string response;

public:
	string define_response_content();
};