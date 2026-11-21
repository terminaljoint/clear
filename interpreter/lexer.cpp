#include <vector>
#include <string>
#include <cctype>
using namespace std;

vector<string> lex(const string& code) {
    vector<string> tokens;
    string current;
    bool inString=false, escaped=false, inComment=false;

    auto flush=[&](){ if(!current.empty()){tokens.push_back(current);current.clear();} };

    for(size_t i=0;i<code.size();++i){
        char c=code[i];
        if(inComment){
            if(c=='\'' && i+2<code.size() && code[i+1]=='\'' && code[i+2]=='\''){
                inComment=false; i+=2; continue;
            }
            if(c=='\n') tokens.push_back("\n");
            continue;
        }
        if(inString){
            current+=c;
            if(escaped) escaped=false;
            else if(c=='\\') escaped=true;
            else if(c=='"'){inString=false;flush();}
            continue;
        }
        if(c=='\'' && i+2<code.size() && code[i+1]=='\'' && code[i+2]=='\''){
            flush(); inComment=true; i+=2; continue;
        }
        if(c=='"'){flush();current+=c;inString=true;continue;}
        if(c=='\n'){flush();tokens.push_back("\n");continue;}
        if(isspace(static_cast<unsigned char>(c))){flush();continue;}
        if(c==';' || c==':' || c=='#' || c==',' || c=='?' || c=='@'){
            flush();
            string tok(1,c);
            if(c=='?' && i+1<code.size() && code[i+1]=='?'){tok="??";++i;}
            if(c=='@'){
                while(tok.size()<3 && i+1<code.size() && code[i+1]=='@'){tok+='@';++i;}
            }
            tokens.push_back(tok);continue;
        }
        if(c=='>' || c=='<' || c=='=' || c=='!'){
            flush(); string op(1,c);
            if(i+1<code.size() && code[i+1]=='='){op+='=';++i;}
            tokens.push_back(op);continue;
        }
        if(c=='+' || c=='-' || c=='*' || c=='/' || c=='%' || c=='(' || c==')'){
            flush();tokens.push_back(string(1,c));continue;
        }
        current+=c;
    }
    flush();
    return tokens;
}
