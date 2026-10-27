#include <http/http.hpp>

using namespace std;

int main() {
    string request = "POST /submit?param1=value1 HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Content-Length: 3\r\n"
                     "\r\n"
                     "body";

    http::parse_request(request);

    return 0;
}
