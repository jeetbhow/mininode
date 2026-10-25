#include <iostream>

#include <http/version.hpp>

int main() {
    std::cout << "Hello from http " << http::version() << "!\n";
    return 0;
}
