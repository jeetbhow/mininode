#include "http/http.hpp"

#include <CLI/CLI.hpp>

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

using namespace std;
using namespace http;

int main(int argc, char** argv) {
    CLI::App app{"An app that tests the functionality of mininode"};

    string filepath;
    app.add_option("--parse", filepath, "Parse a request and print the request object.");

    CLI11_PARSE(app, argc, argv);

    if (!filepath.empty()) {
        ifstream file(filepath);
        if (!file) {
            cerr << "Could not open " << filepath << "\n";
            return 1;
        }

        stringstream buf;
        buf << file.rdbuf();

        // Normalize to CRLF whether the file uses LF or CRLF line endings.
        string request_raw = buf.str();
        string request_lf = regex_replace(request_raw, regex("\r\n"), "\n");
        string request_processed = regex_replace(request_lf, regex("\n"), "\r\n");

        expected<Request, ParseError> r = http::parse_request(request_processed);
        if (r) {
            cout << r->to_string() << "\n";
        } else {
            cout << "Failed to parse request." << "\n";
        }
    }

    return 0;
}
