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


class ExprAST {
 public:
    virtual ~ExprAST() {};
};

class NumberExprAST:ExprAST {
private:
    double Val;
public:
    NumberExprAST(double Val):Val(Val) {}
};

class VariableExprAST:ExprAST {
private:
    std::string Name;
public:
    VariableExprAST(const std::string &name) : Name(name) {}
};

class BinaryExprAST:ExprAST {
private:
    char Op;
    /* what do the * mean here?*/
    ExprAST *LHS, *RHS;
public:
    BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs) : Op(op), LHS(lhs), RHS(rhs) {}
};

class CallExprAST:ExprAST {
private:
    std::string Callee;
    std::vector<ExprAST*> Args;
public:
    CallExprAST(const std::string Callee, std::vector<ExprAST*> Args) : Callee(Callee), Args(Args) {}
};

class PrototypeAST /* note this does not inherit from ExprAST */ {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &Name, const std::vector<std::string> &args) : Name(Name), Args(args) {}
};

class FunctionAST { 
    /* a function has a prototype and a body! this is getting cool!!!! */
    PrototypeAST *Proto;
    ExprAST *Body;
public:
    /* QUESTION: what do * and & mean in the arg list. and why are some things const and others not? */
    FunctionAST(PrototypeAST *Proto, ExprAST *body) : Proto(Proto), Body(body) {}
};
    
static int CurTok;

static int getNextToken() {
    return CurTok = gettok();
}

ExprAST *error(const char *Str) {
    fprintf(stderr, "Error %s\n", Str);
    return 0;
}

