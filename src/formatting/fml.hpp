#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace fml {

    // Section -> key -> list of values
    using FmlData = std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>>;

    inline std::string encode_fml(const FmlData& data) {
        std::string fml_data;

        for (const auto& section_pair : data) {
            const std::string& section = section_pair.first;
            const auto& inner_map = section_pair.second;

            fml_data += "<" + section + ">\n";

            for (const auto& kv : inner_map) {
                const std::string& key = kv.first;
                const auto& values = kv.second;

                for (const auto& value : values) {
                    fml_data += "[" + key + "] \"" + value + "\"\n";
                }
            }

            fml_data += "\n";
        }
        return fml_data;
    }

    inline FmlData decode_fml(const std::string& fml_data) {
        FmlData data;

        bool in_label = false;
        bool in_key = false;
        bool in_string = false;
        bool toggle_string = false;

        std::string currLabel = "";
        std::string currKey = "";
        std::string tok = "";

        char prevC = 0;

        for (char c : fml_data) {
            if (in_label && c == '>') {
                in_label = false;
                currLabel = tok;
                tok = "";
            }
            else if (in_key && c == ']') {
                in_key = false;
                currKey = tok;
                tok = "";
            }
            else if (in_string && c == '"') {
                toggle_string = true;
                data[currLabel][currKey].push_back(tok);
                tok = "";
            }

            if (!(in_label || in_key || in_string)) {
                if (c == '<') in_label = true;
                else if (c == '[') in_key = true;
                else if (c == '"') in_string = true;
            }

            if (toggle_string) {
                in_string = false;
                toggle_string = false;
            }

            if ((in_label && c != '<') || (in_key && c != '[') || (in_string && (c != '"' && prevC != '\\'))) {
                tok += c;
            }

            prevC = c;
        }
        return data;
    }
};
