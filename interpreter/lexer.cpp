#include <vector>
#include <string>
#include <cctype>
#include <stdexcept>
using namespace std;
vector<string> lex(const string&code){
 vector<string>t;string cur;bool inStr=false,escape=false,comment=false;
 auto flush=[&](){if(!cur.empty()){t.push_back(cur);cur.clear();}};
 for(size_t i=0;i<code.size();i++){
  char c=code[i];
  if(comment){if(c=='\''&&i+2<code.size()&&code[i+1]=='\''&&code[i+2]=='\''){comment=false;i+=2;continue;}if(c=='\n')t.push_back("\n");continue;}
  if(inStr){cur+=c;if(escape)escape=false;else if(c=='\\')escape=true;else if(c=='"'){inStr=false;flush();}continue;}
  if(c=='\''&&i+2<code.size()&&code[i+1]=='\''&&code[i+2]=='\''){flush();comment=true;i+=2;continue;}
  if(c=='"'){flush();cur+=c;inStr=true;continue;}
  if(c=='\n'){flush();t.push_back("\n");continue;}
  if(isspace((unsigned char)c)){flush();continue;}
  if(c==';'||c==':'||c=='#'||c==','||c=='?'||c=='@'){
   flush();string s(1,c);
   if(c=='?'&&i+1<code.size()&&code[i+1]=='?'){s="??";i++;}
   if(c=='@'){while(s.size()<3&&i+1<code.size()&&code[i+1]=='@'){s+='@';i++;}if(s=="@@@"&&i+1<code.size()&&(code[i+1]=='m'||code[i+1]=='f'))s+=code[++i];}
   t.push_back(s);continue;
  }
  if(c=='>'||c=='<'||c=='='||c=='!'){flush();string s(1,c);if(i+1<code.size()&&code[i+1]=='='){s+='=';i++;}t.push_back(s);continue;}
  if(c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='('||c==')'){flush();t.push_back(string(1,c));continue;}
  cur+=c;
 }
 flush();if(inStr)throw runtime_error("Unterminated string literal");if(comment)throw runtime_error("Unterminated triple-quote comment");return t;
}
