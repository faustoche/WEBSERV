#include "server.hpp"

int main() {
    c_server server;
    c_location loc;
    
    // Configuration de la location
    loc.set_url_key("/images/");
    loc.set_root("./www/images");
    loc.set_is_directory(true);
    
    // Ajout à la map
    server.add_location("/images/", loc);
    
    std::cout << "=== Test 1: /images/test.jpg ===" << std::endl;
    c_location* result1 = server.find_matching_location("/images/test.jpg");
    
    std::cout << "\n=== Test 2: /images/ ===" << std::endl;
    c_location* result2 = server.find_matching_location("/images/");
    
    std::cout << "\n=== Test 3: /other/path ===" << std::endl;
    c_location* result3 = server.find_matching_location("/other/path");
    
    std::cout << "\n=== Résultats ===" << std::endl;
    std::cout << "Test 1: " << (result1 ? "TROUVÉ" : "PAS TROUVÉ") << std::endl;
    std::cout << "Test 2: " << (result2 ? "TROUVÉ" : "PAS TROUVÉ") << std::endl;
    std::cout << "Test 3: " << (result3 ? "TROUVÉ" : "PAS TROUVÉ") << std::endl;
    
    return 0;
}
