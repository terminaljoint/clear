
#include <vector>
#include <string>
using namespace std;

vector<vector<string>> commands;

void parse(const vector<string>& tokens) {
    commands.clear();

    vector<string> current;

    for (const string& token : tokens) {
        if (token == "\n" || token == ";") {
            if (!current.empty()) {
                commands.push_back(current);
                current.clear();
            }
        }
        else {
            current.push_back(token);
        }
    }

    // Handle final statement if file doesn't end with newline
    if (!current.empty()) {
        commands.push_back(current);
    }
}
