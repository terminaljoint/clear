
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <cstdlib>
using namespace std;

extern vector<vector<string>> commands;

map<string, int> vars;

// Expression evaluator
class Expression {
    const vector<string>& tokens;
    size_t pos;

public:
    Expression(const vector<string>& t, size_t start)
        : tokens(t), pos(start) {}

    int evaluate() {
        int result = expr();

        if (pos != tokens.size())
            throw runtime_error("Unexpected token: " + tokens[pos]);

        return result;
    }

private:
    int expr() {
        int value = term();

        while (pos < tokens.size()) {
            if (tokens[pos] == "+") {
                pos++;
                value += term();
            }
            else if (tokens[pos] == "-") {
                pos++;
                value -= term();
            }
            else {
                break;
            }
        }

        return value;
    }

    int term() {
        int value = factor();

        while (pos < tokens.size()) {
            if (tokens[pos] == "*") {
                pos++;
                value *= factor();
            }
            else if (tokens[pos] == "/") {
                pos++;
                int divisor = factor();

                if (divisor == 0)
                    throw runtime_error("Division by zero");

                value /= divisor;
            }
            else if (tokens[pos] == "%") {
                pos++;
                int divisor = factor();

                if (divisor == 0)
                    throw runtime_error("Modulo by zero");

                value %= divisor;
            }
            else {
                break;
            }
        }

        return value;
    }

    int factor() {
        if (pos >= tokens.size())
            throw runtime_error("Expected a value");

        string token = tokens[pos++];

        // Parentheses
        if (token == "(") {
            int value = expr();

            if (pos >= tokens.size() || tokens[pos] != ")")
                throw runtime_error("Missing closing parenthesis");

            pos++;
            return value;
        }

        // Unary minus
        if (token == "-")
            return -factor();

        // Unary plus
        if (token == "+")
            return factor();

        // Integer literal
        char* end = nullptr;
        long number = strtol(token.c_str(), &end, 10);

        if (end != token.c_str() && *end == '\0')
            return static_cast<int>(number);

        // Variable
        auto it = vars.find(token);

        if (it != vars.end())
            return it->second;

        throw runtime_error("Unknown value: " + token);
    }
};

void execute() {
    vars.clear();

    for (const auto& cmd : commands) {
        try {
            if (cmd.empty())
                continue;

            // Assignment: name = expression
            if (cmd.size() >= 3 && cmd[1] == "=") {
                string name = cmd[0];

                vector<string> expressionTokens(
                    cmd.begin() + 2, cmd.end()
                );

                Expression expression(expressionTokens, 0);
                vars[name] = expression.evaluate();
            }

            // Print: print expression
            else if (cmd[0] == "print" && cmd.size() >= 2) {
                vector<string> expressionTokens(
                    cmd.begin() + 1, cmd.end()
                );

                Expression expression(expressionTokens, 0);
                cout << expression.evaluate() << endl;
            }

            else {
                throw runtime_error("Invalid statement");
            }
        }
        catch (const exception& e) {
            cout << "CLEAR error: " << e.what() << endl;
        }
    }
}
