#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <exception>
#include "ast.h"
using namespace std;
vector<string> lex(const string&code);
void parse(const vector<string>&tokens,vector<ClearNode>&nodes);
void execute(const vector<ClearNode>&nodes);
int main(int argc,char*argv[]){if(argc<2){cerr<<"No file provided\n";return 1;}ifstream file(argv[1]);if(!file){cerr<<"File not found: "<<argv[1]<<"\n";return 1;}stringstream buffer;buffer<<file.rdbuf();try{vector<string>tokens=lex(buffer.str());vector<ClearNode>nodes;parse(tokens,nodes);execute(nodes);}catch(const exception&e){cerr<<"CLEAR syntax error: "<<e.what()<<"\n";return 1;}return 0;}
