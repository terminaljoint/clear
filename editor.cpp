#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cctype>
#include <sstream>

using namespace std;

struct Pos { int row, col; };
static vector<string> code = {""};
static int cx = 0, cy = 0, firstRow = 0;
static bool selecting = false, dirty = false, outputVisible = true;
static Pos anchor{0,0};
static vector<string> output, errors;
static int outputFirstRow=0;
static bool outputFocused=false, outputSelecting=false;
static Pos outputAnchor{0,0}, outputCursor{0,0};
static const vector<string> keywords = {"print","loop","if","elif","else","input","return"};
static const int fontSize = 17, lineHeight = 24, gutter = 58;

static Pos startSel() {
    Pos a=anchor, b{cy,cx};
    if(a.row>b.row || (a.row==b.row && a.col>b.col)) swap(a,b);
    return a;
}
static Pos endSel() {
    Pos a=anchor, b{cy,cx};
    if(a.row>b.row || (a.row==b.row && a.col>b.col)) swap(a,b);
    return b;
}
static bool hasSel() { return selecting && (anchor.row!=cy || anchor.col!=cx); }
static void clearSel() { selecting=false; anchor={cy,cx}; }
static const vector<string>& visibleOutput() { return errors.empty()?output:errors; }
static Pos outputStartSel() { Pos a=outputAnchor,b=outputCursor; if(a.row>b.row||(a.row==b.row&&a.col>b.col)) swap(a,b); return a; }
static Pos outputEndSel() { Pos a=outputAnchor,b=outputCursor; if(a.row>b.row||(a.row==b.row&&a.col>b.col)) swap(a,b); return b; }
static bool hasOutputSel() { return outputSelecting&&(outputAnchor.row!=outputCursor.row||outputAnchor.col!=outputCursor.col); }
static string outputSelectionText() { if(!hasOutputSel()) return ""; const auto& lines=visibleOutput(); Pos a=outputStartSel(),b=outputEndSel(); if(lines.empty()) return ""; a.row=max(0,min(a.row,(int)lines.size()-1)); b.row=max(0,min(b.row,(int)lines.size()-1)); if(a.row==b.row) return lines[a.row].substr(a.col,max(0,b.col-a.col)); string s=lines[a.row].substr(a.col)+"\n"; for(int r=a.row+1;r<b.row;r++) s+=lines[r]+"\n"; s+=lines[b.row].substr(0,b.col); return s; }
static int outputColAtX(const sf::Font& font,const string& s,float target) { float w=0; int c=0; while(c<(int)s.size()&&w<target) { float adv=font.getGlyph((unsigned char)s[c],15,false).advance; if(w+adv/2>=target) break; w+=adv;c++; } return c; }
static string getSelText() {
    if(!hasSel()) return "";
    Pos a=startSel(), b=endSel();
    if(a.row==b.row) return code[a.row].substr(a.col,b.col-a.col);
    string s=code[a.row].substr(a.col)+"\n";
    for(int r=a.row+1;r<b.row;r++) s+=code[r]+"\n";
    s+=code[b.row].substr(0,b.col);
    return s;
}
static void eraseSel() {
    if(!hasSel()) return;
    Pos a=startSel(), b=endSel();
    a.col=max(a.col,indentOf(a.row));
    if(a.row==b.row) code[a.row].erase(a.col,b.col-a.col);
    else {
        string merged=code[a.row].substr(0,a.col)+code[b.row].substr(b.col);
        code.erase(code.begin()+a.row+1,code.begin()+b.row+1);
        code[a.row]=merged;
    }
    cy=a.row; cx=a.col; clearSel(); dirty=true;
}
static void insert(const string& s) {
    eraseSel();
    clampCursor();
    for(char ch:s) {
        if(ch=='\r') continue;
        if(ch=='\n') {
            string tail=code[cy].substr(cx);
            code[cy]=code[cy].substr(0,cx);
            code.insert(code.begin()+cy+1,tail);
            cy++; cx=0;
        } else {
            code[cy].insert(cx,1,ch); cx++;
        }
    }
    clearSel(); dirty=true;
}
static int indentOf(int row) { int n=0; while(n<(int)code[row].size() && code[row][n]==' ') n++; return n; }
static void clampCursor() { cy=max(0,min(cy,(int)code.size()-1)); cx=max(indentOf(cy),min(cx,(int)code[cy].size())); }
static bool keyword(const string& w) {
    return find(keywords.begin(),keywords.end(),w)!=keywords.end();
}
static bool saveFile() {
    ofstream f("temp.clear");
    if(!f) { errors={"Unable to write temp.clear"}; return false; }
    for(const auto& line:code) f<<line<<"\n";
    f.close(); dirty=false; return true;
}
static vector<string> runCmd(const string& cmd) {
    vector<string> result;
    FILE* p=popen(cmd.c_str(),"r");
    if(!p) return {"Could not start interpreter"};
    char buf[512];
    while(fgets(buf,sizeof(buf),p)) {
        string s(buf);
        if(!s.empty() && s.back()=='\n') s.pop_back();
        result.push_back(s);
    }
    pclose(p);
    return result;
}
static vector<string> validateBlocks() {
    vector<string> e; int depth=0;
    for(size_t i=0;i<code.size();i++) {
        bool inString=false;
        for(char ch:code[i]) {
            if(ch=='"') inString=!inString;
            if(inString) continue;
            if(ch==':') depth++;
            if(ch=='#') depth--;
            if(depth<0) { e.push_back("Line "+to_string(i+1)+": unexpected #"); return e; }
        }
    }
    if(depth>0) e.push_back("Unclosed block: missing #");
    return e;
}
static void runCode() {
    errors=validateBlocks();
    output.clear();
    if(!errors.empty()) return;
    if(!saveFile()) return;
    output=runCmd("./interpreter/clear_interpreter temp.clear");
}
static bool loadFont(sf::Font& font) {
    const char* paths[]={
        "/usr/share/fonts/google-noto-vf/NotoSansMono[wght].ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/NotoSansMono-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf"
    };
    for(const char* p:paths) if(font.loadFromFile(p)) return true;
    return false;
}
static float textWidth(const sf::Font& font,const string& s) {
    float w=0;
    for(unsigned char ch:s) w+=font.getGlyph(ch,fontSize,false).advance;
    return w;
}
static int colAtX(const sf::Font& font,const string& s,float target) {
    float w=0; int c=0;
    while(c<(int)s.size() && w<target) {
        float a=font.getGlyph((unsigned char)s[c],fontSize,false).advance;
        if(w+a/2>=target) break;
        w+=a; c++;
    }
    return c;
}
static void drawText(sf::RenderWindow& win,const sf::Font& font,const string& s,float x,float y,sf::Color color,unsigned size=fontSize) {
    sf::Text t(s,font,size); t.setFillColor(color); t.setPosition(x,y); win.draw(t);
}
static sf::Color tokenColor(const string& word) {
    if(keyword(word)) return sf::Color(100,205,245);
    if(!word.empty() && isdigit((unsigned char)word[0])) return sf::Color(190,220,130);
    return sf::Color(225,230,238);
}
static void renderCodeLine(sf::RenderWindow& win,const sf::Font& font,const string& line,float x,float y) {
    float px=x; string word; bool str=false;
    auto flush=[&]() {
        if(word.empty()) return;
        drawText(win,font,word,px,y,tokenColor(word));
        px+=textWidth(font,word); word.clear();
    };
    for(char ch:line) {
        if(ch=='"') {
            flush();
            drawText(win,font,string(1,ch),px,y,sf::Color(230,165,95));
            px+=font.getGlyph(ch,fontSize,false).advance; str=!str;
        } else if(str) {
            drawText(win,font,string(1,ch),px,y,sf::Color(230,165,95));
            px+=font.getGlyph((unsigned char)ch,fontSize,false).advance;
        } else if(isspace((unsigned char)ch) || ch==':' || ch=='#' || ch=='@' || ch=='(' || ch==')' || ch==',' || ch=='?' || ch=='=') {
            flush();
            sf::Color c=(ch==':'||ch=='#')?sf::Color(235,195,85):(ch=='@'?sf::Color(190,130,255):sf::Color(180,190,205));
            drawText(win,font,string(1,ch),px,y,c);
            px+=font.getGlyph((unsigned char)ch,fontSize,false).advance;
        } else word+=ch;
    }
    flush();
}
struct Button {
    sf::FloatRect bounds; string label; bool active=false;
    bool hit(float x,float y) const { return bounds.contains(x,y); }
    void draw(sf::RenderWindow& w,const sf::Font& f) const {
        sf::RectangleShape r({bounds.width,bounds.height});
        r.setPosition(bounds.left,bounds.top);
        r.setFillColor(active?sf::Color(48,90,115):sf::Color(45,52,64));
        r.setOutlineThickness(1); r.setOutlineColor(sf::Color(70,82,98)); w.draw(r);
        sf::Text t(label,f,14); t.setFillColor(sf::Color(225,230,238));
        auto b=t.getLocalBounds(); t.setPosition(bounds.left+(bounds.width-b.width)/2- b.left,bounds.top+(bounds.height-b.height)/2-b.top);
        w.draw(t);
    }
};
int main() {
    sf::RenderWindow win(sf::VideoMode(1100,760),"CLEAR IDE");
    win.setFramerateLimit(60);
    sf::Font font;
    if(!loadFont(font)) { fprintf(stderr,"CLEAR: Could not load a monospace font.\n"); return 1; }
    Button runBtn, saveBtn, outputBtn;
    sf::Clock blink; bool cursorOn=true;
    const sf::Color bg(18,21,27), panel(25,29,37), toolbar(31,36,45), muted(120,132,148);
    while(win.isOpen()) {
        float W=(float)win.getSize().x, H=(float)win.getSize().y;
        const auto& outLines=visibleOutput();
        int outRows=min(9,max(4,(int)outLines.size()));
        float outputH=outputVisible?min(H*0.40f,max(130.f,44.f+outRows*22.f)):0.f;
        float editorTop=76.f, editorBottom=H-outputH-12.f;
        if(editorBottom<editorTop+80) editorBottom=editorTop+80;
        float outputTop=H-outputH;
        runBtn.bounds={W-232,14,94,34}; runBtn.label="Run  F5";
        saveBtn.bounds={W-334,14,92,34}; saveBtn.label="Save";
        outputBtn.bounds={W-130,14,116,34}; outputBtn.label=outputVisible?"Hide  F4":"Output  F4";
        sf::Event ev;
        while(win.pollEvent(ev)) {
            if(ev.type==sf::Event::Closed) win.close();
            if(ev.type==sf::Event::Resized) {
                win.setView(sf::View(sf::FloatRect(0,0,ev.size.width,ev.size.height)));
            }
            if(ev.type==sf::Event::MouseWheelScrolled) {
                float mx=ev.mouseWheelScroll.x,my=ev.mouseWheelScroll.y;
                if(outputVisible && my>=outputTop) { const auto& ls=visibleOutput(); int visibleRows=max(1,(int)((outputH-40)/22)); outputFirstRow=max(0,min(max(0,(int)ls.size()-visibleRows),outputFirstRow-(int)ev.mouseWheelScroll.delta)); }
                else if(mx<gutter+8 || my<editorBottom) firstRow=max(0,firstRow-(int)ev.mouseWheelScroll.delta);
            }
            if(ev.type==sf::Event::MouseButtonPressed && ev.mouseButton.button==sf::Mouse::Left) {
                float mx=ev.mouseButton.x,my=ev.mouseButton.y;
                if(runBtn.hit(mx,my)) { runCode(); continue; }
                if(saveBtn.hit(mx,my)) { if(saveFile()) errors.clear(); continue; }
                if(outputBtn.hit(mx,my)) { outputVisible=!outputVisible; outputFocused=false; outputSelecting=false; continue; }
                if(outputVisible && my>=outputTop+30 && my<outputTop+outputH) { const auto& lines=visibleOutput(); if(!lines.empty()){ int visibleRows=max(1,(int)((outputH-40)/22)); int row=max(0,min(visibleRows-1,(int)((my-outputTop-36)/22.f)))+outputFirstRow; row=min(row,(int)lines.size()-1); outputCursor={row,outputColAtX(font,lines[row],mx-18)}; outputAnchor=outputCursor; outputSelecting=true; outputFocused=true; } continue; }
                if(my>=editorTop && my<editorBottom && mx>=gutter) { outputFocused=false; outputSelecting=false;
                    int row=firstRow+(int)((my-editorTop)/lineHeight);
                    row=max(0,min(row,(int)code.size()-1));
                    float textX=mx-gutter-12;
                    cy=row; cx=max(indentOf(row),colAtX(font,code[row],textX));
                    clearSel();
                }
            }
            if(ev.type==sf::Event::MouseMoved && outputSelecting && sf::Mouse::isButtonPressed(sf::Mouse::Left) && outputVisible) { float mx=ev.mouseMove.x,my=ev.mouseMove.y; const auto& lines=visibleOutput(); if(!lines.empty()&&my>=outputTop+30&&my<outputTop+outputH){int visibleRows=max(1,(int)((outputH-40)/22)); int row=max(0,min(visibleRows-1,(int)((my-outputTop-36)/22.f)))+outputFirstRow; row=min(row,(int)lines.size()-1); outputCursor={row,outputColAtX(font,lines[row],mx-18)};} }
            if(ev.type==sf::Event::KeyPressed) {
                bool ctrl=ev.key.control, shift=ev.key.shift;
                if(ctrl && ev.key.code==sf::Keyboard::S) { if(saveFile()) errors.clear(); continue; }
                if(ev.key.code==sf::Keyboard::F4) { outputVisible=!outputVisible; continue; }
                if(ev.key.code==sf::Keyboard::F5) { runCode(); continue; }
                if(ctrl && ev.key.code==sf::Keyboard::A) { if(outputFocused){const auto& lines=visibleOutput();if(!lines.empty()){int last=(int)lines.size()-1;outputAnchor={0,0};outputCursor={last,(int)lines[last].size()};outputSelecting=true;} } else { anchor={0,0}; cy=(int)code.size()-1; cx=(int)code.back().size(); selecting=true; } continue; }
                if(ctrl && ev.key.code==sf::Keyboard::C) { if(outputFocused&&hasOutputSel()) sf::Clipboard::setString(outputSelectionText()); else if(hasSel()) sf::Clipboard::setString(getSelText()); continue; }
                if(outputFocused) continue;
                if(ctrl && ev.key.code==sf::Keyboard::X) { if(hasSel()) { sf::Clipboard::setString(getSelText()); eraseSel(); } continue; }
                if(ctrl && ev.key.code==sf::Keyboard::V) { insert(sf::Clipboard::getString()); continue; }
                int oldX=cx,oldY=cy;
                if(ev.key.code==sf::Keyboard::Left) { if(cx>indentOf(cy)) cx--; }
                else if(ev.key.code==sf::Keyboard::Right) { if(cx<(int)code[cy].size()) cx++; else if(cy+1<(int)code.size()){cy++;cx=indentOf(cy);} }
                else if(ev.key.code==sf::Keyboard::Up) { if(cy>0) cy--; cx=max(indentOf(cy),min(cx,(int)code[cy].size())); }
                else if(ev.key.code==sf::Keyboard::Down) { if(cy+1<(int)code.size()) cy++; cx=max(indentOf(cy),min(cx,(int)code[cy].size())); }
                else if(ev.key.code==sf::Keyboard::Home) cx=indentOf(cy);
                else if(ev.key.code==sf::Keyboard::End) cx=(int)code[cy].size();
                else if(ev.key.code==sf::Keyboard::BackSpace) { if(hasSel()) eraseSel(); else if(cx>indentOf(cy)){code[cy].erase(cx-1,1);cx--;dirty=true;} }
                else if(ev.key.code==sf::Keyboard::Delete) { if(hasSel()) eraseSel(); else if(cx<(int)code[cy].size()){code[cy].erase(cx,1);dirty=true;} else if(cy+1<(int)code.size()){code[cy]+=code[cy+1];code.erase(code.begin()+cy+1);dirty=true;} }
                else if(ev.key.code==sf::Keyboard::Tab) insert("    ");
                else if(ev.key.code==sf::Keyboard::Enter) {
                    string left=code[cy].substr(0,cx),right=code[cy].substr(cx);
                    int indent=0; while(indent<(int)left.size() && left[indent]==' ') indent++;
                    if(!left.empty() && left.back()==':') indent+=4;
                    else if(!left.empty() && left.back()=='#') indent=max(0,indent-4);
                    code[cy]=left; code.insert(code.begin()+cy+1,string(indent,' ')+right);
                    cy++; cx=indent; clearSel(); dirty=true;
                }
                bool moved=(ev.key.code==sf::Keyboard::Left||ev.key.code==sf::Keyboard::Right||ev.key.code==sf::Keyboard::Up||ev.key.code==sf::Keyboard::Down||ev.key.code==sf::Keyboard::Home||ev.key.code==sf::Keyboard::End);
                if(shift && moved) { if(!selecting) {anchor={oldY,oldX};selecting=true;} }
                else if(moved) clearSel();
                clampCursor();
                if(cy<firstRow) firstRow=cy;
                int visible=max(1,(int)((editorBottom-editorTop)/lineHeight));
                if(cy>=firstRow+visible) firstRow=cy-visible+1;
            }
            if(ev.type==sf::Event::TextEntered) {
                uint32_t ch=ev.text.unicode;
                if(ch>=32 && ch<127 && ch!='\t') insert(string(1,(char)ch));
            }
        }
        if(blink.getElapsedTime().asMilliseconds()>500) { cursorOn=!cursorOn; blink.restart(); }
        win.clear(bg);
        // Top toolbar
        sf::RectangleShape top({W,60}); top.setPosition(0,0); top.setFillColor(toolbar); win.draw(top);
        drawText(win,font,"CLEAR",18,18,sf::Color(110,200,245),21);
        drawText(win,font,"IDE",84,23,muted,13);
        sf::RectangleShape tab({150,32}); tab.setPosition(18,78); tab.setFillColor(sf::Color(36,42,52)); tab.setOutlineThickness(1); tab.setOutlineColor(sf::Color(60,72,86)); win.draw(tab);
        drawText(win,font,string("temp.clear")+(dirty?"  *":""),30,86,sf::Color(215,222,232),14);
        saveBtn.draw(win,font); runBtn.draw(win,font); outputBtn.draw(win,font);
        // Editor body
        sf::RectangleShape editor({W-24,editorBottom-editorTop}); editor.setPosition(12,editorTop); editor.setFillColor(panel); win.draw(editor);
        for(int r=firstRow;r<(int)code.size();r++) {
            float y=editorTop+8+(r-firstRow)*lineHeight;
            if(y>editorBottom-lineHeight) break;
            sf::Text ln(to_string(r+1),font,14); ln.setFillColor(muted); ln.setPosition(20,y+2); win.draw(ln);
            if(hasSel()) {
                Pos a=startSel(),b=endSel();
                if(r>=a.row && r<=b.row) {
                    int s=(r==a.row?a.col:0),e=(r==b.row?b.col:(int)code[r].size());
                    float x=gutter+12+textWidth(font,code[r].substr(0,s));
                    float ww=textWidth(font,code[r].substr(s,max(0,e-s)));
                    if(r!=b.row) ww+=8;
                    sf::RectangleShape h({max(ww,4.f),lineHeight-2}); h.setPosition(x,y); h.setFillColor(sf::Color(62,91,132,170)); win.draw(h);
                }
            }
            renderCodeLine(win,font,code[r],gutter+12,y);
        }
        // Cursor
        if(cursorOn && cy>=firstRow && cy<firstRow+(int)((editorBottom-editorTop)/lineHeight)) {
            float x=gutter+12+textWidth(font,code[cy].substr(0,cx));
            float y=editorTop+8+(cy-firstRow)*lineHeight;
            sf::RectangleShape caret({2,19}); caret.setPosition(x,y+1); caret.setFillColor(sf::Color(220,230,245)); win.draw(caret);
        }
        // Output area
        if(outputVisible) {
            sf::RectangleShape outPanel({W,outputH}); outPanel.setPosition(0,outputTop); outPanel.setFillColor(sf::Color(22,25,31)); win.draw(outPanel);
            sf::RectangleShape divider({W,1}); divider.setPosition(0,outputTop); divider.setFillColor(sf::Color(65,75,88)); win.draw(divider);
            drawText(win,font,"OUTPUT",18,outputTop+12,sf::Color(135,150,170),13);
            drawText(win,font,errors.empty()?"":"ERRORS",95,outputTop+12,sf::Color(220,105,105),13);
            const auto& lines=visibleOutput(); int outputRows=max(1,(int)((outputH-40)/22)); for(int i=outputFirstRow;i<(int)lines.size()&&i<outputFirstRow+outputRows;i++){ float y=outputTop+36+(i-outputFirstRow)*22; if(hasOutputSel()){Pos a=outputStartSel(),b=outputEndSel();if(i>=a.row&&i<=b.row){int st=i==a.row?a.col:0,en=i==b.row?b.col:(int)lines[i].size();float x=18+textWidth(font,lines[i].substr(0,st));float ww=textWidth(font,lines[i].substr(st,max(0,en-st)));if(i!=b.row)ww+=5;sf::RectangleShape hl({max(ww,3.f),19});hl.setPosition(x,y-1);hl.setFillColor(sf::Color(55,85,120,180));win.draw(hl);}} drawText(win,font,lines[i],18,y,errors.empty()?sf::Color(145,220,155):sf::Color(235,115,115),15); }
        }
        // Status bar
        sf::RectangleShape status({W,24}); status.setPosition(0,H-24); status.setFillColor(sf::Color(34,40,49)); win.draw(status);
        drawText(win,font,dirty?"Modified":"Saved",12,H-20,dirty?sf::Color(230,180,90):muted,12);
        drawText(win,font,"Ln "+to_string(cy+1)+"  Col "+to_string(cx+1)+"     F5 Run   F4 Output   Ctrl+S Save",W-390,H-20,muted,12);
        win.display();
    }
    return 0;
}
