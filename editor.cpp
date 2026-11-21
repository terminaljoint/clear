
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cctype>

using namespace std;

// ---------- KEYWORDS ----------
vector<string> keywords = {"print","loop","if","elif","else"};

// ---------- POSITION ----------
struct Position {
    int y, x;
};

vector<string> lines = {""};
int cursorX = 0, cursorY = 0;
int scroll = 0;

bool selecting = false;
Position anchor = {0, 0};

// ---------- SELECTION ----------
Position normalizedStart() {
    Position a = anchor, b = {cursorY, cursorX};

    if (a.y > b.y || (a.y == b.y && a.x > b.x))
        swap(a, b);

    return a;
}

Position normalizedEnd() {
    Position a = anchor, b = {cursorY, cursorX};

    if (a.y > b.y || (a.y == b.y && a.x > b.x))
        swap(a, b);

    return b;
}

bool hasSelection() {
    return selecting && (anchor.y != cursorY || anchor.x != cursorX);
}

void clearSelection() {
    selecting = false;
    anchor = {cursorY, cursorX};
}

string selectedText() {
    if (!hasSelection()) return "";

    Position a = normalizedStart();
    Position b = normalizedEnd();

    string result;

    if (a.y == b.y)
        return lines[a.y].substr(a.x, b.x-a.x);

    result += lines[a.y].substr(a.x) + "\n";

    for (int i = a.y+1; i < b.y; i++)
        result += lines[i] + "\n";

    result += lines[b.y].substr(0,b.x);

    return result;
}

void deleteSelection() {
    if (!hasSelection()) return;

    Position a = normalizedStart();
    Position b = normalizedEnd();

    if (a.y == b.y) {
        lines[a.y].erase(a.x,b.x-a.x);
    }
    else {
        string merged = lines[a.y].substr(0,a.x) + lines[b.y].substr(b.x);

        lines.erase(lines.begin()+a.y+1,lines.begin()+b.y+1);
        lines[a.y] = merged;
    }

    cursorY = a.y;
    cursorX = a.x;

    clearSelection();
}

// ---------- INSERT TEXT ----------
void insertText(const string& text) {
    deleteSelection();

    for(char c : text) {
        if(c == '\r') continue;

        if(c == '\n') {
            string right = lines[cursorY].substr(cursorX);
            lines[cursorY] = lines[cursorY].substr(0,cursorX);

            lines.insert(lines.begin()+cursorY+1,right);

            cursorY++;
            cursorX = 0;
        }
        else {
            lines[cursorY].insert(cursorX,1,c);
            cursorX++;
        }
    }

    clearSelection();
}

// ---------- RUN ----------
vector<string> runInterpreter(const string& cmd){
    vector<string> out;

    FILE* pipe = popen(cmd.c_str(),"r");

    if(!pipe){
        out.push_back("Interpreter failed");
        return out;
    }

    char buf[256];

    while(fgets(buf,sizeof(buf),pipe)){
        string s(buf);
        s.erase(remove(s.begin(),s.end(),'\n'),s.end());
        out.push_back(s);
    }

    pclose(pipe);
    return out;
}

// ---------- ERROR ----------
vector<string> checkErrors(const vector<string>& lines){
    vector<string> err;
    int b=0;

    for(int i=0;i<lines.size();i++){
        if(lines[i].find(':')!=string::npos) b++;
        if(lines[i].find('#')!=string::npos) b--;

        if(b<0){
            err.push_back("Line "+to_string(i+1)+": extra #");
            return err;
        }
    }

    if(b!=0) err.push_back("Missing # (block not closed)");

    return err;
}

// ---------- KEYWORD ----------
bool isKeyword(const string& word){
    return find(keywords.begin(),keywords.end(),word)!=keywords.end();
}

// ---------- DRAW ----------
void drawLine(sf::RenderWindow& w,sf::Font& f,const string& line,float x,float y){

    float off=x;
    string word="";
    bool inString=false;

    for(int i=0;i<=line.size();i++){
        char ch=(i<line.size()?line[i]:' ');

        if(isspace(ch)||ch==':'||ch=='#'||ch=='@'||ch=='"'||i==line.size()){

            if(!word.empty()){
                sf::Text t(word,f,18);

                if(isKeyword(word)) t.setFillColor(sf::Color::Cyan);
                else if(isdigit(word[0])) t.setFillColor(sf::Color::Green);
                else t.setFillColor(sf::Color::White);

                t.setPosition(off,y);
                w.draw(t);

                for(char c:word)
                    off+=f.getGlyph(c,18,false).advance;

                word.clear();
            }

            if(i<line.size()){
                sf::Text t(string(1,ch),f,18);

                if(ch=='"'){
                    inString=!inString;
                    t.setFillColor(sf::Color(255,165,0));
                }
                else if(inString)
                    t.setFillColor(sf::Color(255,165,0));
                else if(ch==':'||ch=='#')
                    t.setFillColor(sf::Color::Yellow);
                else if(ch=='@')
                    t.setFillColor(sf::Color(200,100,255));
                else
                    t.setFillColor(sf::Color::White);

                t.setPosition(off,y);
                w.draw(t);

                off+=f.getGlyph(ch,18,false).advance;
            }
        }
        else word+=ch;
    }
}

// ---------- MAIN ----------
int main(){

    sf::RenderWindow window(sf::VideoMode(1000,720),"CLEAR IDE");

    sf::Font font;

    if(!font.loadFromFile("/usr/share/fonts/google-noto-vf/NotoSansMono[wght].ttf"))
        return -1;

    vector<string> outputLines,errorLines;
    vector<int> indentStack;

    vector<string> suggestions;
    int selectedSuggestion=0;

    sf::Clock blink;
    bool showCursor=true;

    while(window.isOpen()){

        sf::Event e;

        while(window.pollEvent(e)){

            if(e.type==sf::Event::Closed)
                window.close();

            // ---------- SCROLL ----------
            if(e.type==sf::Event::MouseWheelScrolled){
                scroll=max(0,scroll-(int)e.mouseWheelScroll.delta);
            }

            // ---------- KEYBOARD ----------
            if(e.type==sf::Event::KeyPressed){

                bool ctrl=e.key.control;
                bool shift=e.key.shift;

                // CTRL+A
                if(ctrl && e.key.code==sf::Keyboard::A){
                    anchor={0,0};
                    cursorY=lines.size()-1;
                    cursorX=lines.back().size();
                    selecting=true;
                    continue;
                }

                // CTRL+C
                if(ctrl && e.key.code==sf::Keyboard::C){
                    if(hasSelection())
                        sf::Clipboard::setString(selectedText());
                    continue;
                }

                // CTRL+X
                if(ctrl && e.key.code==sf::Keyboard::X){
                    if(hasSelection()){
                        sf::Clipboard::setString(selectedText());
                        deleteSelection();
                    }
                    continue;
                }

                // CTRL+V
                if(ctrl && e.key.code==sf::Keyboard::V){
                    insertText(sf::Clipboard::getString());
                    continue;
                }

                // F5 RUN
                if(e.key.code==sf::Keyboard::F5){

                    errorLines=checkErrors(lines);

                    if(errorLines.empty()){
                        ofstream t("temp.clear");

                        for(auto &l:lines)
                            t<<l<<"\n";

                        t.close();

                        outputLines = runInterpreter(
    "/home/murugansanthosh/Win11BU/driveda-1/chromebook-linux/clear/interpreter/clear_interpreter /home/murugansanthosh/Win11BU/driveda-1/chromebook-linux/clear/cmake-build-debug/temp.clear");
                    }

                    continue;
                }

                // AUTOCOMPLETE
                if(e.key.code==sf::Keyboard::Enter && !suggestions.empty()){

                    int s=cursorX-1;

                    while(s>=0 && lines[cursorY][s]!=' ')
                        s--;

                    lines[cursorY].replace(s+1,cursorX-s-1,suggestions[selectedSuggestion]);

                    cursorX=s+1+suggestions[selectedSuggestion].size();

                    suggestions.clear();
                    clearSelection();
                    continue;
                }

                // MOVEMENT
                int oldX=cursorX, oldY=cursorY;

                if(e.key.code==sf::Keyboard::Left){
                    if(cursorX>0) cursorX--;
                    else if(cursorY>0){
                        cursorY--;
                        cursorX=lines[cursorY].size();
                    }
                }

                if(e.key.code==sf::Keyboard::Right){
                    if(cursorX<lines[cursorY].size()) cursorX++;
                    else if(cursorY<lines.size()-1){
                        cursorY++;
                        cursorX=0;
                    }
                }

                if(e.key.code==sf::Keyboard::Up && cursorY>0)
                    cursorY--;

                if(e.key.code==sf::Keyboard::Down && cursorY<lines.size()-1)
                    cursorY++;

                if(e.key.code==sf::Keyboard::Home)
                    cursorX=0;

                if(e.key.code==sf::Keyboard::End)
                    cursorX=lines[cursorY].size();

                // SELECTION WITH SHIFT
                if(shift){
                    if(!selecting){
                        anchor={oldY,oldX};
                        selecting=true;
                    }
                }
                else if(e.key.code==sf::Keyboard::Left ||
                        e.key.code==sf::Keyboard::Right ||
                        e.key.code==sf::Keyboard::Up ||
                        e.key.code==sf::Keyboard::Down ||
                        e.key.code==sf::Keyboard::Home ||
                        e.key.code==sf::Keyboard::End){
                    clearSelection();
                }

                // DELETE SELECTION
                if(hasSelection() &&
                   (e.key.code==sf::Keyboard::BackSpace ||
                    e.key.code==sf::Keyboard::Delete)){
                    deleteSelection();
                    continue;
                }

                // BACKSPACE
                if(e.key.code==sf::Keyboard::BackSpace && !hasSelection()){
                    if(cursorX>0){
                        lines[cursorY].erase(cursorX-1,1);
                        cursorX--;
                    }
                    else if(cursorY>0){
                        cursorX=lines[cursorY-1].size();
                        lines[cursorY-1]+=lines[cursorY];
                        lines.erase(lines.begin()+cursorY);
                        cursorY--;
                    }
                }

                // DELETE
                if(e.key.code==sf::Keyboard::Delete && !hasSelection()){
                    if(cursorX<lines[cursorY].size())
                        lines[cursorY].erase(cursorX,1);
                    else if(cursorY<lines.size()-1){
                        lines[cursorY]+=lines[cursorY+1];
                        lines.erase(lines.begin()+cursorY+1);
                    }
                }

                // TAB
                if(e.key.code==sf::Keyboard::Tab){
                    insertText("    ");
                }

                // ENTER
                if(e.key.code==sf::Keyboard::Enter && suggestions.empty()){
                    // Enter is handled in TextEntered
                }

                // Escape selection
                if(e.key.code==sf::Keyboard::Escape){
                    clearSelection();
                    suggestions.clear();
                }
            }

            // ---------- TEXT ----------
            if(e.type==sf::Event::TextEntered){

                char c=e.text.unicode;

                int indent=0;

                // Calculate current indentation
                while(indent<lines[cursorY].size() &&
                      lines[cursorY][indent]==' ')
                    indent++;

                // ENTER
                if(c==13){

                    string left=lines[cursorY].substr(0,cursorX);
                    string right=lines[cursorY].substr(cursorX);

                    int newIndent=indent;

                    if(!left.empty() && left.back()==':')
                        newIndent+=4;

                    if(!left.empty() && left.back()=='#')
                        newIndent=max(0,newIndent-4);

                    lines[cursorY]=left;

                    string nl(newIndent,' ');
                    nl+=right;

                    lines.insert(lines.begin()+cursorY+1,nl);

                    cursorY++;
                    cursorX=newIndent;

                    clearSelection();
                }

                // NORMAL TEXT
                else if(c>=32 && c<127){
                    insertText(string(1,c));
                }

                // AUTOCOMPLETE
                suggestions.clear();

                int s=cursorX-1;

                while(s>=0 && lines[cursorY][s]!=' ')
                    s--;

                string w=lines[cursorY].substr(s+1,cursorX-s-1);

                for(auto &k:keywords)
                    if(k.find(w)==0 && w!="")
                        suggestions.push_back(k);

                selectedSuggestion=0;
            }
        }

        // ---------- SCROLL ----------
        if(cursorY-scroll>20) scroll++;
        if(cursorY<scroll) scroll=cursorY;

        // ---------- BLINK ----------
        if(blink.getElapsedTime().asMilliseconds()>500){
            showCursor=!showCursor;
            blink.restart();
        }

        // ---------- DRAW ----------
        window.clear(sf::Color(20,20,20));

        // SELECTION
        if(hasSelection()){
            Position a=normalizedStart();
            Position b=normalizedEnd();

            for(int row=a.y;row<=b.y;row++){

                int start=(row==a.y?a.x:0);
                int end=(row==b.y?b.x:lines[row].size());

                float x=50;

                for(int i=0;i<start;i++)
                    x+=font.getGlyph(lines[row][i],18,false).advance;

                float width=0;

                for(int i=start;i<end;i++)
                    width+=font.getGlyph(lines[row][i],18,false).advance;

                if(row!=b.y) width+=10;

                sf::RectangleShape highlight(sf::Vector2f(max(width,5.f),20));
                highlight.setFillColor(sf::Color(70,100,150,180));
                highlight.setPosition(x,40+(row-scroll)*22);

                window.draw(highlight);
            }
        }

        // CODE
        for(int i=scroll;i<lines.size();i++){
            float y=40+(i-scroll)*22;
            if(y>480) break;

            drawLine(window,font,lines[i],50,y);
        }

        // CURSOR
        if(showCursor){

            float cx=50;

            for(int i=0;i<cursorX;i++)
                cx+=font.getGlyph(lines[cursorY][i],18,false).advance;

            float cy=40+(cursorY-scroll)*22;

            sf::RectangleShape cur(sf::Vector2f(2,18));
            cur.setPosition(cx,cy);
            window.draw(cur);
        }

        // ---------- OUTPUT PANEL ----------
        sf::RectangleShape panel(sf::Vector2f(1000,200));
        panel.setPosition(0,500);
        panel.setFillColor(sf::Color(30,30,30));
        window.draw(panel);

        sf::Text outputTitle("OUTPUT  |  F5 = RUN",font,14);
        outputTitle.setFillColor(sf::Color(150,150,150));
        outputTitle.setPosition(10,505);
        window.draw(outputTitle);

        // OUTPUT
        for(int i=0;i<outputLines.size()&&i<6;i++){
            sf::Text t(outputLines[i],font,16);
            t.setFillColor(sf::Color::Green);
            t.setPosition(10,530+i*20);
            window.draw(t);
        }

        // ERRORS
        for(int i=0;i<errorLines.size()&&i<6;i++){
            sf::Text t(errorLines[i],font,16);
            t.setFillColor(sf::Color::Red);
            t.setPosition(10,650+i*20);
            window.draw(t);
        }

        window.display();
    }

    return 0;
}
