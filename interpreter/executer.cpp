#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <cstdlib>
using namespace std;

extern vector<vector<string>> commands;

struct Value {
    bool isString;
    int number;
    string text;

    Value(int n = 0) : isString(false), number(n) {}
    Value(const string& s) : isString(true), number(0), text(s) {}
};

map<string, Value> vars;

static string valueToString(const Value& value) {
    return value.isString ? value.text : to_string(value.number);
}

static string decodeString(const string& token) {
    string result;
    for (size_t i = 1; i + 1 < token.size(); i++) {
        if (token[i] == '\\' && i + 2 < token.size()) {
            char next = token[++i];
            if (next == 'n') result += '\n';
            else if (next == 't') result += '\t';
            else result += next;
        } else {
            result += token[i];
        }
    }
    return result;
}

class Expression {
    const vector<string>& tokens;
    size_t pos;

public:
    Expression(const vector<string>& t, size_t start)
        : tokens(t), pos(start) {}

    Value evaluate() {
        Value result = expr();
        if (pos != tokens.size())
            throw runtime_error("Unexpected token: " + tokens[pos]);
        return result;
    }

private:
    Value expr() {
        Value left = arithmetic();

        while (pos < tokens.size()) {
            string op = tokens[pos];
            if (op != ">" && op != "<" && op != ">=" && op != "<=" &&
                op != "==" && op != "!=")
                break;

            pos++;
            Value right = arithmetic();
            bool result = false;

            if (left.isString != right.isString)
                throw runtime_error("Cannot compare a string with a number");

            if (left.isString) {
                if (op == "==") result = left.text == right.text;
                else if (op == "!=") result = left.text != right.text;
                else if (op == ">") result = left.text > right.text;
                else if (op == "<") result = left.text < right.text;
                else if (op == ">=") result = left.text >= right.text;
                else if (op == "<=") result = left.text <= right.text;
            } else {
                if (op == "==") result = left.number == right.number;
                else if (op == "!=") result = left.number != right.number;
                else if (op == ">") result = left.number > right.number;
                else if (op == "<") result = left.number < right.number;
                else if (op == ">=") result = left.number >= right.number;
                else if (op == "<=") result = left.number <= right.number;
            }

            left = Value(result ? 1 : 0);
        }
        return left;
    }

    Value arithmetic() {
        Value value = term();

        while (pos < tokens.size()) {
            if (tokens[pos] == "+") {
                pos++;
                Value right = term();
                if (value.isString || right.isString)
                    value = Value(valueToString(value) + valueToString(right));
                else
                    value.number += right.number;
            }
            else if (tokens[pos] == "-") {
                if (value.isString)
                    throw runtime_error("Cannot subtract from a string");
                pos++;
                Value right = term();
                if (right.isString)
                    throw runtime_error("Cannot subtract a string");
                value.number -= right.number;
            }
            else break;
        }
        return value;
    }

    Value term() {
        Value value = factor();

        while (pos < tokens.size()) {
            if (tokens[pos] == "*") {
                if (value.isString)
                    throw runtime_error("Cannot multiply a string");
                pos++;
                Value right = factor();
                if (right.isString)
                    throw runtime_error("Cannot multiply by a string");
                value.number *= right.number;
            }
            else if (tokens[pos] == "/") {
                if (value.isString)
                    throw runtime_error("Cannot divide a string");
                pos++;
                Value right = factor();
                if (right.isString)
                    throw runtime_error("Cannot divide by a string");
                if (right.number == 0)
                    throw runtime_error("Division by zero");
                value.number /= right.number;
            }
            else if (tokens[pos] == "%") {
                if (value.isString)
                    throw runtime_error("Cannot use modulo on a string");
                pos++;
                Value right = factor();
                if (right.isString)
                    throw runtime_error("Cannot use modulo with a string");
                if (right.number == 0)
                    throw runtime_error("Modulo by zero");
                value.number %= right.number;
            }
            else break;
        }
        return value;
    }

    Value factor() {
        if (pos >= tokens.size())
            throw runtime_error("Expected a value");

        string token = tokens[pos++];

        if (token == "(") {
            Value value = expr();
            if (pos >= tokens.size() || tokens[pos] != ")")
                throw runtime_error("Missing closing parenthesis");
            pos++;
            return value;
        }

        if (token == "-") {
            Value value = factor();
            if (value.isString)
                throw runtime_error("Cannot negate a string");
            value.number = -value.number;
            return value;
        }
        if (token == "+") return factor();

        if (token.size() >= 2 && token.front() == '"' && token.back() == '"')
            return Value(decodeString(token));

        char* end = nullptr;
        long number = strtol(token.c_str(), &end, 10);
        if (end != token.c_str() && *end == '\0')
            return Value(static_cast<int>(number));

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
            if (cmd.empty()) continue;

            // Assignment: name = expression
            if (cmd.size() >= 3 && cmd[1] == "=") {
                string name = cmd[0];
                vector<string> expressionTokens(cmd.begin() + 2, cmd.end());
                Expression expression(expressionTokens, 0);
                vars[name] = expression.evaluate();
            }
            // Print: print expression
            else if (cmd[0] == "print") {
                if (cmd.size() == 1) {
                    cout << endl;
                } else {
                    vector<string> expressionTokens(cmd.begin() + 1, cmd.end());
                    Expression expression(expressionTokens, 0);
                    cout << valueToString(expression.evaluate()) << endl;
                }
            }
            // Input: input variableName (stores the line as a string)
            else if (cmd[0] == "input" && cmd.size() == 2) {
                string inputText;
                getline(cin, inputText);
                vars[cmd[1]] = Value(inputText);
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
