#include <http/http.hpp>

using namespace std;

int main() {
    string request = "GET / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "\r\n";    
    http::parse_request(request);

    return 0;
}
