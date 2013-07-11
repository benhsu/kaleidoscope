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

class NumberExprAST: public ExprAST {
private:
    double Val;
public:
    NumberExprAST(double Val):Val(Val) {}
};

class VariableExprAST: public ExprAST {
private:
    std::string Name;
public:
    VariableExprAST(const std::string* name) : Name(name) {}
};

class BinaryExprAST: public ExprAST {
private:
    char Op;
    ExprAST *LHS;
    ExprAST *RHS;
public:
    BinaryExprAST(char op, ExprAST* lhs, ExprAST* rhs) : Op(op), LHS(lhs), RHS(rhs) {}
};

class CallExprAST: public ExprAST {
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

/* error handling is always useful! */
ExprAST *Error(const char *Str) {
    fprintf(stderr, "Error %s\n", Str);
    return 0;
}

PrototypeAST *ErrorP(const char *Str) { Error(Str); return 0;}

/* needs different name otherwise compiler wont be able to tell which one was called*/
FunctionAST *ErrorF(const char *Str) {Error(Str); return 0;}

// called when current token is a number. expects NumVal to be populated by gettok()
static ExprAST *ParseNumberExpr() {
    ExprAST *Result = new NumberExprAST(NumVal); /* numval is global */
    getNextToken(); // munch 
    return Result;
}

// parenexpr = '(' + expr + ')'
static ExprAST *ParseParenExpr() {
    getNextToken(); // eat the '('
    ExprAST *V = ParseExpression(); // to be populated, recursive descent!
    if (!V) return 0; // error? or ()
    if (CurTok!=')') {
        return Error("Expected ')'");
    }
    getNextToken(); // eat the ')'
    return V;
}

// we have one function to resolve a variable ref AND a function call?
// could this be because jmc is a genius and realized they are both s-expressions? probably not

static ExprAST *ParseIdentifierExpr() {
    std::string Idname = IdentifierStr; // this is populated by gettok()
    getNextToken(); // eat the identifier
    if (CurTok != '(') { // use look ahead to check next token!
        // simple identifier name
        return new VariableExprAST(Idname);
    }
    getNextToken(); // eat '('
    std::vector<ExprAST*> Args;
    if(CurTok!=')') /* seems clumsy */ {
        while(1) {
            ExprAST *Arg = ParseExpression(); // keep in mind each argument can be a function call!
            if(!Arg) return 0;
            Args.push_back(Arg);
            if(CurTok==')') break; /* WTF is this shit? */
            if(CurTok!=',') return Error("Expected ')' or ','");
            getNextToken();
        }
    }
    getNextToken(); //eat the ')'
    return new CallExprAST(Idname, Args);
}

// a "primary expression" is a identifier (lval), a number (rval), or a paren expression??? 
// this will become clear later. have faith in the tutorial

static ExprAST *ParsePrimary() {
    switch(CurTok) {
    case tok_identifier: return ParseIdentifierExpr();
    case tok_number: return ParseNumberExpr();
    case '(': return ParseParenExpr();
    default: return Error("I dont know what this token is");
    }
}
