// Build host views of live kernel code without copying implementations into tests.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;
using std::string;
fs::path repository, output;

string read(const fs::path& path)
{
    std::ifstream input(path);
    if(!input) throw std::runtime_error("cannot read " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

string source(const string& path)
{
    static std::unordered_map<string, string> cache;
    auto found = cache.find(path);
    if(found != cache.end()) return found->second;
    std::istringstream input(read(repository / path));
    string result, current;
    bool first = true;
    while(std::getline(input, current))
    {
        if(!current.empty() && current.back() == '\r') current.pop_back();
        if(current.compare(0, 8, "#include") == 0) continue;
        if(!first) result += '\n';
        result += current;
        first = false;
    }
    cache[path] = result;
    return result;
}

size_t find(const string& text, const string& marker, size_t start = 0)
{
    auto position = text.find(marker, start);
    if(position == string::npos) throw std::runtime_error("missing source marker: " + marker);
    return position;
}

size_t rfind(const string& text, const string& marker, size_t end = string::npos)
{
    if(end == string::npos) end = text.size();
    auto position = text.substr(0, end).rfind(marker);
    if(position == string::npos) throw std::runtime_error("missing source marker: " + marker);
    return position;
}

string slice(const string& text, size_t start, size_t end = string::npos)
{
    if(end == string::npos) end = text.size();
    if(start > end || end > text.size()) throw std::runtime_error("invalid source range");
    return text.substr(start, end - start);
}

string fromSignature(const string& text, const string& marker) { return slice(text, find(text, marker)); }
string before(const string& text, const string& marker) { return slice(text, 0, find(text, marker)); }
string after(const string& text, const string& marker) { return slice(text, find(text, marker) + marker.size()); }
string between(const string& text, const string& begin, const string& end)
{
    auto start = find(text, begin);
    return slice(text, start, find(text, end, start));
}

string replace(string text, const string& from, const string& to)
{
    size_t position = 0;
    // A missing replacement is allowed for optional instruction spellings (pause).
    while((position = text.find(from, position)) != string::npos)
    {
        text.replace(position, from.size(), to);
        position += to.size();
    }
    return text;
}

// Ignore braces inside comments and string/character literals when finding a body.
string function(const string& text, const string& signature)
{
    auto start = find(text, signature), opening = find(text, "{", start);
    unsigned depth = 0;
    char quoted = 0;
    bool lineComment = false, blockComment = false;
    for(size_t i = opening; i < text.size(); ++i)
    {
        char c = text[i], next = i + 1 < text.size() ? text[i + 1] : '\0';
        if(lineComment) { if(c == '\n') lineComment = false; continue; }
        if(blockComment) { if(c == '*' && next == '/') { blockComment = false; ++i; } continue; }
        if(quoted)
        {
            if(c == '\\') ++i;
            else if(c == quoted) quoted = 0;
            continue;
        }
        if(c == '/' && next == '/') { lineComment = true; ++i; continue; }
        if(c == '/' && next == '*') { blockComment = true; ++i; continue; }
        if(c == '\'' || c == '"') { quoted = c; continue; }
        if(c == '{') ++depth;
        else if(c == '}' && --depth == 0) return slice(text, start, i + 1);
    }
    throw std::runtime_error("unclosed function: " + signature);
}

string replaceFunction(string text, const string& signature, const string& replacement)
{
    auto body = function(text, signature);
    text.replace(find(text, signature), body.size(), replacement);
    return text;
}

string defines(const string& text, const string& prefix)
{
    std::istringstream input(text);
    string result, current;
    while(std::getline(input, current))
        if(current.compare(0, prefix.size() + 8, "#define " + prefix) == 0)
            result += current + '\n';
    if(result.empty()) throw std::runtime_error("missing macro prefix: " + prefix);
    return result;
}

string callstruct(const string& text, const string& name, bool packed)
{
    auto marker = "}__attribute__((packed)) " + name + ";";
    auto end = find(text, marker), start = rfind(text, "typedef struct", end);
    return slice(text, start, end) + (packed ? marker : "} " + name + ";");
}

void emit(const string& name, const string& content)
{
    auto path = output / name;
    auto complete = content + '\n';
    if(fs::exists(path) && read(path) == complete) return;
    std::ofstream file(path);
    if(!file || !(file << complete)) throw std::runtime_error("cannot write " + path.string());
}

int main(int argc, char** argv)
{
    if(argc != 3) { std::cerr << "usage: extract_sources REPOSITORY OUTPUT\n"; return 1; }
    try
    {
        repository = argv[1]; output = argv[2];
        fs::create_directories(output);
#include "source_recipes.inc"
    }
    catch(const std::exception& error)
    {
        std::cerr << "kernel test source extraction: " << error.what() << '\n';
        return 1;
    }
}
