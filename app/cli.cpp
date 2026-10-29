#include "http/http.hpp"
#include "http/router.hpp"

#include <CLI/CLI.hpp>

#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>

using namespace std;
using namespace http;

int main(int argc, char** argv) {
    CLI::App app{"An app that tests the functionality of mininode"};
    app.require_subcommand(1);

    CLI::App* parse_cmd = app.add_subcommand("parse", "Parse a request and print the request object");
    CLI::App* route_cmd = app.add_subcommand("route", "Test a route against target");

    // Parse
    string filepath;
    parse_cmd->add_option("filepath", filepath, "Parse a request and print the request object");
    
    // Route
    pair<string, string> route;
    pair<string, string> target;    
    route_cmd->add_option("route", route, "The route being tested against")->required();
    route_cmd->add_option("target", target, "The target to match for")->required();

    CLI11_PARSE(app, argc, argv);

    if (*parse_cmd) {
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

    if (*route_cmd) {
        auto [route_method_raw, route_path] = route;
        auto [target_method_raw, target_path] = target;

        optional<Method> route_method = parse_method(route_method_raw);
        if (!route_method) {
            cerr << "route has invalid method\n";
            return 1;
        }

        optional<Method> target_method = parse_method(target_method_raw);
        if (!target_method) {
            cerr << "target has invalid method\n";
            return 1;
        }

        Router router{};
        router.add(*route_method, route_path);
        
        if (router.match(*target_method, target_path)) {
            cout << "It's a match!\n";
        } else {
            cout << "It's not a match.\n";
        }

        return 0;
    }

    return 0;
}
