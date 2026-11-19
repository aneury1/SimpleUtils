#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace simpleutils{




struct ArgumentHandler {
    std::unordered_map<std::string, std::vector<std::string>> _args;
    std::string _separator;

    ArgumentHandler(int argc, char *argv[], const std::string &prefix = "--")
        : _separator(prefix) {
        std::string current; // "" = positionals before any option
        for (int i = 1; i < argc; ++i) {
            std::string tok = argv[i];
            if (!_separator.empty() && tok.size() > _separator.size() &&
                tok.compare(0, _separator.size(), _separator) == 0) {
                std::string key = tok.substr(_separator.size());
                auto eq = key.find('=');
                if (eq != std::string::npos) {           // --key=value
                    current = key.substr(0, eq);
                    _args[current].push_back(key.substr(eq + 1));
                } else {                                  // --key [values...]
                    current = key;
                    _args[current]; // create entry (flag with no values)
                }
            } else {
                _args[current].push_back(tok);
            }
        }
    }

    std::optional<std::vector<std::string>> getValue(const std::string &opt) {
        auto it = _args.find(opt);
        if (it == _args.end()) return std::nullopt;
        return it->second;
    }
};

};