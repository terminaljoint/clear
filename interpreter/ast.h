#ifndef CLEAR_AST_H
#define CLEAR_AST_H
#include <string>
#include <vector>
struct ClearNode {
    enum Type { Statement, If, Loop, Function, Module } type = Statement;
    std::vector<std::string> tokens;
    std::vector<ClearNode> body;
    struct Branch { std::vector<std::string> condition; std::vector<ClearNode> body; bool isElse=false; };
    std::vector<Branch> branches;
    std::string name;
    std::vector<std::string> params;
    std::vector<std::string> startExpr, endExpr;
};
#endif
