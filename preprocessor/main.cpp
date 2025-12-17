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

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

string GetFileContents(string file) {
    ifstream stream(file);
    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

string GetFileContents(path file) {
    ifstream stream(file);
    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

string FindIncludeFile(string& str) {
    static regex include_regex_file(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    smatch m;
    if (regex_match(str, m, include_regex_file)) {
        return string(m[1]);
    }
    return {};
}

string FindIncludeHeader(string& str) {
    static regex include_regex_header(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch m;
    if (regex_match(str, m, include_regex_header)) {
        return string(m[1]);
    }
    return {};
}

// const path MakePath(const path& in_file, const string& str) {
//     return path(in_file.parent_path()) / path(str);
// }

const path SearchDirectories (const string& str, const vector<path>& include_directories) {         //search for include in vector<path>& include_directories
    //cout << "DEBUG: What to find: " << path(str).string() << ", " << path(str).filename().string() << endl;
    for (auto& item : include_directories) {
        //cout << "DEBUG: " << item.string() << endl;
        auto file_path = item / path(str);
        if (filesystem::exists(file_path)) {
            return file_path;
            //cout << dir_entry.path().string() << endl;
        }
    }
    return path();
}

const path CheckFilePath(const path& outside, const path& inside = path()) {            //checks file paths and returns valid one if exists
    if (filesystem::exists(inside)) {
        return inside;
    }
    else if (filesystem::exists(outside)) {
        return outside;
    }
    else {
        return path();
    }
}

bool PreprocessInternal(ifstream& in_stream, ofstream& out_stream, const path& in_file, const vector<path>& include_directories) {
    if (!filesystem::exists(in_file)) {
        return false;
    }
    in_stream.open(in_file);
    if (!in_stream.is_open()) {
        return false;
    }

    int line_count = 0;         //count lines received from in_file
    while (in_stream) {
        string string_from_file{};
        std::getline(in_stream, string_from_file);
        line_count++;

        if (string_from_file.empty() && in_stream.eof()) {                  //if string is empty and reached end of file - return TRUE
            //cout << "EOF: " << in_file.filename().string() << endl;
            return true;
        }

        string check_include_file = FindIncludeFile(string_from_file);          //check if string contains #include "..."
        string check_include_header = FindIncludeHeader(string_from_file);      //check if string contains #include <...>

        bool result_include_process = true;
        string result_include_name{};
        ifstream input_stream;
        if (check_include_file.empty() && check_include_header.empty()) {       //if string contains NO #include - write to file
            out_stream << string_from_file << endl;
            continue;
        }
        if (!check_include_file.empty()) {              //if string contains #include "..."
            auto file_search_inside = in_file.parent_path() / path(check_include_file);
            auto file_search_outside = SearchDirectories(check_include_file, include_directories);
            auto next_path_file = CheckFilePath(file_search_outside, file_search_inside);
            if (next_path_file.empty()) {               //if paths NOT valid
                result_include_process = false;
                result_include_name = check_include_file;
            }
            else {
                result_include_process = PreprocessInternal(input_stream, out_stream, next_path_file, include_directories);
            }
        }
        if (!check_include_header.empty()) {            //if string contains #include <...>
            auto header_search_outside = SearchDirectories(check_include_header, include_directories);
            auto next_path_header = CheckFilePath(header_search_outside);
            if (next_path_header.empty()) {             //if paths NOT valid
                result_include_process = false;
                result_include_name = check_include_header;
            }
            else {
                result_include_process = PreprocessInternal(input_stream, out_stream, next_path_header, include_directories);
            }
        }
        if (!result_include_process) {          //if NO file exists write error message
            cout << "unknown include file " << result_include_name << " at file " <<
                in_file.string() << " at line " << line_count << endl;
            return false;
        }
    }
    return false;
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    if (!filesystem::exists(in_file)) {
        return false;
    }
    ifstream input_file;
    ofstream output_file(out_file, ios::app);
    if (!output_file.is_open()) {
        return false;
    }
    return PreprocessInternal(input_file, output_file, in_file, include_directories);          //process original file with recursive call
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                        {"sources"_p / "include1"_p,"sources"_p / "include2"_p});
    // assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
    //                     {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
