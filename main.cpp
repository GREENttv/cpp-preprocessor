#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, size_t sz) {
    return path(data, data + sz);
}

void PrintError(const string& inc_file, const path& current_file, int line_number) {
    cout << "unknown include file " << inc_file
         << " at file " << current_file.string()
         << " at line " << line_number << endl;
}

bool PreprocessImpl(const path& current_file, istream& in, ofstream& out,
                    const vector<path>& include_dirs, int& line_number);

bool Preprocess(const path& in_file, const path& out_file,
                const vector<path>& include_dirs) {
    ifstream input(in_file);
    if (!input) {
        cerr << "Failed to open input file: " << in_file.string() << endl;
        return false;
    }
    ofstream output(out_file);
    if (!output) {
        cerr << "Failed to open output file: " << out_file.string() << endl;
        return false;
    }
    int line_number = 1;
    return PreprocessImpl(in_file, input, output, include_dirs, line_number);
}

bool PreprocessImpl(const path& current_file, istream& in, ofstream& out,
                    const vector<path>& include_dirs, int& line_number) {
    static const regex re_quote("^\\s*#\\s*include\\s*\"([^\\\"]*)\"\\s*");
    static const regex re_angle("^\\s*#\\s*include\\s*<([^>]*)>\\s*");
    string line;
    while (true) {
        if (!getline(in, line)) {
            return true;
        }
        smatch match;
        if (regex_match(line, match, re_quote)) {
            auto inc_file = match[1].str();
            path found;
            path local_candidate = current_file.parent_path() / inc_file;
            if (filesystem::exists(local_candidate)) {
                found = local_candidate;
            } else {
                for (const auto& dir : include_dirs) {
                    auto candidate = dir / inc_file;
                    if (filesystem::exists(candidate)) {
                        found = candidate;
                        break;
                    }
                }
            }
            if (found.empty()) {
                PrintError(inc_file, current_file, line_number);
                return false;
            }
            ifstream inc_in(found);
            if (!inc_in) {
                PrintError(inc_file, current_file, line_number);
                return false;
            }
            int sub_line_number = 1;
            if (!PreprocessImpl(found, inc_in, out, include_dirs, sub_line_number)) {
                return false;
            }
        } else if (regex_match(line, match, re_angle)) {
            auto inc_file = match[1].str();
            path found;
            for (const auto& dir : include_dirs) {
                auto candidate = dir / inc_file;
                if (filesystem::exists(candidate)) {
                    found = candidate;
                    break;
                }
            }
            if (found.empty()) {
                PrintError(inc_file, current_file, line_number);
                return false;
            }
            ifstream inc_in(found);
            if (!inc_in) {
                PrintError(inc_file, current_file, line_number);
                return false;
            }
            int sub_line_number = 1;
            if (!PreprocessImpl(found, inc_in, out, include_dirs, sub_line_number)) {
                return false;
            }
        } else {
            out << line << "\n";
        }
        ++line_number;
    }
}