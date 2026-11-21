#include "ast.h"
#include <vector>
#include <string>
#include <stdexcept>
using namespace std;
static vector<string> ts; static size_t p=0;
static void skipSep(){while(p<ts.size()&&(ts[p]=="\\n"||ts[p]==";"))p++;}
static vector<string> takeStatement(){vector<string>a;int par=0;while(p<ts.size()){string t=ts[p];if(par==0&&(t=="\\n"||t==";"||t=="#"))break;if(t=="(")par++;if(t==")")par--;a.push_back(t);p++;}return a;}
static vector<string> takeHeadColon(){vector<string>a;int par=0;while(p<ts.size()){string t=ts[p];if(t==":"&&par==0){p++;break;}if(t=="(")par++;if(t==")")par--;a.push_back(t);p++;}return a;}
static vector<ClearNode> block(bool stopBranches=false){
 vector<ClearNode>out;
 while(p<ts.size()){
  skipSep();if(p>=ts.size()||ts[p]=="#")break;if(stopBranches&&(ts[p]=="elif"||ts[p]=="else"))break;
  if(ts[p]=="if"){p++;ClearNode n;n.type=ClearNode::If;ClearNode::Branch b;b.condition=takeHeadColon();b.body=block(true);n.branches.push_back(b);while(p<ts.size()&&ts[p]=="elif"){p++;ClearNode::Branch e;e.condition=takeHeadColon();e.body=block(true);n.branches.push_back(e);}if(p<ts.size()&&ts[p]=="else"){p++;ClearNode::Branch e;e.isElse=true;takeHeadColon();e.body=block(true);n.branches.push_back(e);}if(p>=ts.size()||ts[p]!="#")throw runtime_error("Missing # to close if block");p++;out.push_back(n);continue;}
  if(ts[p]=="loop"){p++;ClearNode n;n.type=ClearNode::Loop;vector<string>h=takeHeadColon();if(h.size()<5||h[1]!="(")throw runtime_error("Expected loop name (start,end):");n.name=h[0];int comma=-1,close=-1;for(int i=2;i<(int)h.size();i++){if(h[i]==",")comma=i;if(h[i]==")")close=i;}if(comma<0||close<0)throw runtime_error("Loop range must be (start,end)");n.startExpr.assign(h.begin()+2,h.begin()+comma);n.endExpr.assign(h.begin()+comma+1,h.begin()+close);n.body=block();if(p>=ts.size()||ts[p]!="#")throw runtime_error("Missing # to close loop");p++;out.push_back(n);continue;}
  if(ts[p]=="@@@f"||ts[p]=="@@@m"){bool mod=ts[p]=="@@@m";p++;ClearNode n;n.type=mod?ClearNode::Module:ClearNode::Function;vector<string>h=takeHeadColon();if(h.empty())throw runtime_error("Missing definition name");n.name=h[0];size_t open=1;while(open<h.size()&&h[open]!="(")open++;if(open==h.size())throw runtime_error("Definition requires parentheses");for(size_t i=open+1;i<h.size()&&h[i]!=")";i++)if(h[i]!=",")n.params.push_back(h[i]);n.body=block();if(p>=ts.size()||ts[p]!="#")throw runtime_error("Missing # to close definition");p++;out.push_back(n);continue;}
  ClearNode n;n.type=ClearNode::Statement;n.tokens=takeStatement();if(!n.tokens.empty()){out.push_back(n);continue;}if(p<ts.size()&&ts[p]=="#")break;
 }
 return out;
}
void parse(const vector<string>&tokens,vector<ClearNode>&nodes){ts=tokens;p=0;nodes=block();skipSep();if(p<ts.size())throw runtime_error("Unexpected block closing #");}
