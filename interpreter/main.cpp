#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

// function declarations
vector<string> lex(const string& code);
void parse(const vector<string>& tokens);
void execute();

int main(int argc, char* argv[]) {

    if (argc < 2) {
        cout << "No file provided\n";
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cout << "File not found\n";
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string code = buffer.str();

    // 🔥 THIS LINE CREATES TOKENS
    vector<string> tokens = lex(code);


    parse(tokens);

    execute();

    return 0;
}
