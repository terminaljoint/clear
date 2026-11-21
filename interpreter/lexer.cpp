#include <vector>
#include <string>
#include <cctype>
using namespace std;

vector<string> lex(const string& code) {
    vector<string> tokens;
    string current;
    bool inString = false;
    bool escaped = false;

    auto flush = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (size_t i = 0; i < code.size(); i++) {
        char c = code[i];

        if (inString) {
            current += c;
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
                flush();
            }
            continue;
        }

        if (c == '"') {
            flush();
            current += c;
            inString = true;
        }
        else if (c == '\n') {
            flush();
            tokens.push_back("\n");
        }
        else if (isspace(static_cast<unsigned char>(c))) {
            flush();
        }
        else if (c == ';') {
            flush();
            tokens.push_back(";");
        }
        else if (c == '>' || c == '<' || c == '=' || c == '!') {
            flush();
            string op(1, c);
            if (i + 1 < code.size() && code[i + 1] == '=') {
                op += '=';
                i++;
            }
            tokens.push_back(op);
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/' ||
                 c == '%' || c == '(' || c == ')') {
            flush();
            tokens.push_back(string(1, c));
        }
        else {
            current += c;
        }
    }

    flush();
    return tokens;
}
