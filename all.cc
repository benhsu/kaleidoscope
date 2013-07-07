#include <ctype.h>
#include <stdio.h>
#include <string>
// what is the difference between import and include?
#import <vector>

enum Token {
    tok_eof=-1,
    tok_def=-2,
    tok_extern=-3,
    tok_identifier=-4,
    tok_number=-5,
};

static std::string IdentifierStr; // if tok is tok_identifier its a variable name, the varname will be here
static double NumVal; // filled in if tok is tok_number

static int gettok() {
    static int LastChar = ' ';
    while (isspace(LastChar)) {
        LastChar = getchar(); // munch space!!!!
    }

    if (isalpha(LastChar)) {
        // begin the loop to eat an identifier!
        IdentifierStr = LastChar;
        while(isalnum((LastChar = getchar()))) {
            IdentifierStr += LastChar;
        }
        // finished eating!
        if (IdentifierStr == "def") {
            return tok_def;
        }
        if (IdentifierStr == "extern") {
            return tok_extern;
        }
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar=='.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar=='.');
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }
    if (LastChar=='#') {
        do {
            LastChar = getchar();
        } while (LastChar!= EOF && LastChar !='\n' && LastChar != '\r');
        if (LastChar!=EOF)
            return gettok();
    }
    
    if (LastChar == EOF)
        return tok_eof;
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}


class ExprAst {
 public:
    virtual ~ExprAst() {};
};

class NumberExprAst:ExprAst {
private:
    double Val;
public:
    NumberExprAst(double Val):Val(Val) {}
};

class VariableExprAst:ExprAst {
private:
    std::string Name;
public:
    VariableExprAst(const std::string &name) : Name(name) {}
};

class BinaryExprAst:ExprAst {
private:
    char Op;
    /* what do the * mean here?*/
    ExprAst *LHS, *RHS;
public:
    BinaryExprAst(char op, ExprAst *lhs, ExprAst *rhs) : Op(op), LHS(lhs), RHS(rhs) {}
};

class CallExprAst:ExprAst {
private:
    std::string Callee;
    std::vector<ExprAst*> Args;
public:
    CallExprAst(const std::string Callee, std::vector<ExprAst*> Args) : Callee(Callee), Args(Args) {}
};

class PrototypeAst /* note this does not inherit from ExprAst */ {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAst(const std::string &Name, const std::vector<std::string> &args) : Name(Name), Args(args) {}
};

class FunctionAst { 
    /* a function has a prototype and a body! this is getting cool!!!! */
    PrototypeAst *Proto;
    ExprAst *Body;
public:
    /* QUESTION: what do * and & mean in the arg list. and why are some things const and others not? */
    FunctionAst(PrototypeAst *Proto, ExprAst *body) : Proto(Proto), Body(body) {}
};
    
static int CurTok;

static int getNextToken() {
    return CurTok = gettok();
}