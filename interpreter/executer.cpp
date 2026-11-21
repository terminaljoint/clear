#include "ast.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <cstdlib>
using namespace std;
struct Value { bool isString=false; long long number=0; string text; Value(){} Value(long long n):number(n){} Value(const string&s):isString(true),text(s){} };
static string str(const Value&v){return v.isString?v.text:to_string(v.number);}
static bool truth(const Value&v){return v.isString?!v.text.empty():v.number!=0;}
static string unquote(const string&t){string r;for(size_t i=1;i+1<t.size();i++){if(t[i]=='\\'&&i+1<t.size()-1){char c=t[++i];if(c=='n')r+='\n';else if(c=='t')r+='\t';else r+=c;}else r+=t[i];}return r;}
struct Func {vector<string> args;vector<ClearNode> body;string module;};
static vector<map<string,Value>> scopes;static map<string,Func> functions;static map<string,bool> modules;
static Value getv(const string&n){for(auto it=scopes.rbegin();it!=scopes.rend();++it){auto f=it->find(n);if(f!=it->end())return f->second;}return Value(0);}
static void setv(const string&n,const Value&v){if(scopes.empty())scopes.push_back({});scopes.back()[n]=v;}
static Value eval(const vector<string>&);
struct ReturnSignal{Value value;};
static void execNodes(const vector<ClearNode>&);
static Value call(const string&m,const string&n,const vector<Value>&a){string key=m.empty()?n:m+"@"+n;auto f=functions.find(key);if(f==functions.end())throw runtime_error("Unknown function: "+key);if(a.size()!=f->second.args.size())throw runtime_error("Function "+n+" expects "+to_string(f->second.args.size())+" arguments");scopes.push_back({});for(size_t i=0;i<a.size();i++)scopes.back()[f->second.args[i]]=a[i];Value r(0);try{execNodes(f->second.body);}catch(ReturnSignal&x){r=x.value;}scopes.pop_back();return r;}
class Expr{
 const vector<string>&t;size_t i=0;
 public:Expr(const vector<string>&v):t(v){}Value run(){Value v=ternary();if(i<t.size())throw runtime_error("Unexpected expression token: "+t[i]);return v;}
 private:
 Value ternary(){Value c=compare();if(i>=t.size()||t[i]!="?")return c;i++;int split=-1,depth=0,nested=0;for(size_t j=i;j<t.size();j++){if(t[j]=="(")depth++;else if(t[j]==")")depth--;else if(depth==0&&t[j]=="?")nested++;else if(depth==0&&t[j]=="??"){if(nested)nested--;else{split=(int)j;break;}}}if(split<0)throw runtime_error("Ternary requires ??");vector<string>a(t.begin()+i,t.begin()+split),b(t.begin()+split+1,t.end());i=t.size();return truth(c)?eval(a):eval(b);}
 Value compare(){Value l=add();while(i<t.size()){string o=t[i];if(o!="=="&&o!="!="&&o!="<"&&o!="<="&&o!=">"&&o!=">=")break;i++;Value r=add();if(l.isString!=r.isString)throw runtime_error("Cannot compare string with number");bool b=false;if(l.isString){if(o=="==")b=l.text==r.text;else if(o=="!=")b=l.text!=r.text;else if(o=="<")b=l.text<r.text;else if(o==">")b=l.text>r.text;else if(o=="<=")b=l.text<=r.text;else b=l.text>=r.text;}else{if(o=="==")b=l.number==r.number;else if(o=="!=")b=l.number!=r.number;else if(o=="<")b=l.number<r.number;else if(o==">")b=l.number>r.number;else if(o=="<=")b=l.number<=r.number;else b=l.number>=r.number;}l=Value(b?1:0);}return l;}
 Value add(){Value l=mul();while(i<t.size()&&(t[i]=="+"||t[i]=="-")){string o=t[i++];Value r=mul();if(o=="+"){if(l.isString||r.isString)l=Value(str(l)+str(r));else l.number+=r.number;}else{if(l.isString||r.isString)throw runtime_error("Cannot subtract strings");l.number-=r.number;}}return l;}
 Value mul(){Value l=unary();while(i<t.size()&&(t[i]=="*"||t[i]=="/"||t[i]=="%")){string o=t[i++];Value r=unary();if(l.isString||r.isString)throw runtime_error("Arithmetic requires numbers");if((o=="/"||o=="%")&&r.number==0)throw runtime_error("Division by zero");if(o=="*")l.number*=r.number;else if(o=="/")l.number/=r.number;else l.number%=r.number;}return l;}
 Value unary(){if(i<t.size()&&(t[i]=="+"||t[i]=="-")){bool neg=t[i++]=="-";Value v=unary();if(v.isString)throw runtime_error("Unary sign requires number");if(neg)v.number=-v.number;return v;}return primary();}
 Value primary(){if(i>=t.size())throw runtime_error("Expected expression value");string x=t[i++];if(x=="("){Value v=ternary();if(i>=t.size()||t[i++]!=")")throw runtime_error("Missing )");return v;}if(x.size()>=2&&x.front()=='"'&&x.back()=='"')return Value(unquote(x));char*e=nullptr;long long n=strtoll(x.c_str(),&e,10);if(e!=x.c_str()&&*e=='\0')return Value(n);
 if(i<t.size()&&(t[i]=="@"||t[i]=="@@")){string connector=t[i++];if(i>=t.size())throw runtime_error("Connector missing function");string fn=t[i++];if(i>=t.size()||t[i++]!="(")throw runtime_error("Expected function arguments");vector<Value>a;vector<string>cur;int d=0;while(i<t.size()){if(t[i]=="(")d++;if(t[i]==")"&&d==0){if(!cur.empty()){a.push_back(eval(cur));cur.clear();}i++;break;}if(t[i]==")")d--;if(t[i]==","&&d==0){a.push_back(eval(cur));cur.clear();i++;continue;}cur.push_back(t[i++]);}if(!cur.empty())a.push_back(eval(cur));if(connector=="@@"&&!modules.count(x))throw runtime_error("Unknown module: "+x);return call(x,fn,a);}return getv(x);}
};
static Value eval(const vector<string>&t){return t.empty()?Value(0):Expr(t).run();}
static vector<vector<string>> splitTop(const vector<string>&t,const string&d){vector<vector<string>>r;vector<string>c;int depth=0;for(auto&s:t){if(s=="(")depth++;else if(s==")")depth--;if(s==d&&depth==0){r.push_back(c);c.clear();}else c.push_back(s);}r.push_back(c);return r;}
static void execNodes(const vector<ClearNode>&nodes){for(const auto&n:nodes){
 if(n.type==ClearNode::If){for(auto&b:n.branches)if(b.isElse||truth(eval(b.condition))){scopes.push_back({});execNodes(b.body);scopes.pop_back();break;}}
 else if(n.type==ClearNode::Loop){Value a=eval(n.startExpr),b=eval(n.endExpr);if(a.isString||b.isString)throw runtime_error("Loop bounds must be numeric");long long from=a.number,to=b.number,step=from<=to?1:-1;scopes.push_back({});for(long long k=from;step>0?k<=to:k>=to;k+=step){setv(n.name,Value(k));execNodes(n.body);}scopes.pop_back();}
 else if(n.type==ClearNode::Module){modules[n.name]=true;for(auto&ch:n.body)if(ch.type==ClearNode::Function){Func f;f.args=ch.params;f.body=ch.body;f.module=n.name;functions[n.name+"@"+ch.name]=f;}}
 else if(n.type==ClearNode::Function){Func f;f.args=n.params;f.body=n.body;functions[n.name]=f;}
 else{const auto&t=n.tokens;if(t.empty())continue;if(t[0]=="print"){if(t.size()==1)cout<<"\n";else cout<<str(eval(vector<string>(t.begin()+1,t.end())))<<"\n";}
 else if(t[0]=="input"&&t.size()==2){string s;getline(cin,s);setv(t[1],Value(s));}
 else if(t[0]=="return")throw ReturnSignal{eval(vector<string>(t.begin()+1,t.end()))};
 else if(t.size()>=3&&t[1]=="="){auto ps=splitTop(t,",");if(ps.size()==1)setv(t[0],eval(vector<string>(t.begin()+2,t.end())));else for(auto&q:ps){if(q.size()<3||q[1]!="=")throw runtime_error("Invalid multiple assignment");setv(q[0],eval(vector<string>(q.begin()+2,q.end())));}}
 else if(t.size()==1)(void)eval(t);else throw runtime_error("Invalid statement");}
}}
void execute(const vector<ClearNode>&nodes){scopes.clear();scopes.push_back({});functions.clear();modules.clear();try{execNodes(nodes);}catch(ReturnSignal&){cerr<<"CLEAR error: return outside function\n";}catch(const exception&e){cerr<<"CLEAR error: "<<e.what()<<"\n";}}
