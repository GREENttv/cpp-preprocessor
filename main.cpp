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

bool PreprocessImpl(const path& current_file, istream& in, ofstream& out,
                    const vector<path>& include_dirs, int& line_number);

bool Preprocess(const path& in_file, const path& out_file,
                const vector<path>& include_dirs) {
    ifstream input(in_file);
    if (!input) {
        return false;
    }
    ofstream output(out_file);
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
                for (auto& dir : include_dirs) {
                    auto candidate = dir / inc_file;
                    if (filesystem::exists(candidate)) {
                        found = candidate;
                        break;
                    }
                }
            }
            if (found.empty()) {
                cout << "unknown include file " << inc_file
                     << " at file " << current_file.string()
                     << " at line " << line_number << endl;
                return false;
            }
            ifstream inc_in(found);
            if (!inc_in) {
                cout << "unknown include file " << inc_file
                     << " at file " << current_file.string()
                     << " at line " << line_number << endl;
                return false;
            }
            int sub_line_number = 1;
            if (!PreprocessImpl(found, inc_in, out, include_dirs, sub_line_number)) {
                return false;
            }
        } else if (regex_match(line, match, re_angle)) {
            auto inc_file = match[1].str();
            path found;
            for (auto& dir : include_dirs) {
                auto candidate = dir / inc_file;
                if (filesystem::exists(candidate)) {
                    found = candidate;
                    break;
                }
            }
            if (found.empty()) {
                cout << "unknown include file " << inc_file
                     << " at file " << current_file.string()
                     << " at line " << line_number << endl;
                return false;
            }
            ifstream inc_in(found);
            if (!inc_in) {
                cout << "unknown include file " << inc_file
                     << " at file " << current_file.string()
                     << " at line " << line_number << endl;
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
