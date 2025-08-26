#include "server.hpp"

std::string	int_to_string(int n){
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}