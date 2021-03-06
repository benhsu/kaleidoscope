#include <ctype.h>
#include <stdio.h>
#include <string>
// what is the difference between import and include?
#import <vector>
#import <map>

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
    VariableExprAST(const std::string& name) : Name(name) {}
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
static std::map<char, int> BinOpPrecedence;

/* error handling is always useful! */
ExprAST *Error(const char *Str) {
    fprintf(stderr, "Error %s\n", Str);
    return 0;
}

PrototypeAST *ErrorP(const char *Str) { Error(Str); return 0;}

/* needs different name otherwise compiler wont be able to tell which one was called*/
FunctionAST *ErrorF(const char *Str) {Error(Str); return 0;}


static ExprAST *ParseExpression();
static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS);


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

static int GetTokPrecedence() {
    if (!isascii(CurTok)) return -1;
    int TokPrec = BinOpPrecedence[CurTok];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}

static ExprAST *ParseExpression() {
    ExprAST *LHS = ParsePrimary(); // a parimary is either an atom or a paren enclosed thing
    if (!LHS) return 0; // return null if nothing
    return ParseBinOpRHS(0, LHS); // binary, handle the operator and rhs
}

static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
    // returns the structure consisting of LHS BinOp RHS
    // if there is no BinOp it just returns the LHS

    // ExprPrec is the minimum precedence this call can "eat"
    // lets say we have 2 * 3 + 5
    // we would want the 3 bound to the 2, not the 5, because * takes precendence over +
    while (1) {
        // loop until we have gone all the way down the tree!
        // look ahead, do not use getNextToken!
        int TokPrec = GetTokPrecedence();
        if (TokPrec < ExprPrec) return LHS; // per above example, LHS=3, BinOp=+, RHS=5, since binop has low precedence do not eat binop or rhs and just return the 3
        // this above is the only non-null return value! its when we've completely eaten the rhs
        int BinOp = CurTok;
        getNextToken();
        // parse the thing to the right of the BinOp
        // note that RHS may be bound to this, or the next one!
        ExprAST *RHS = ParsePrimary();
        if (!RHS) return 0; // no RHS. error
    
        // now we have a RHS and a LHS. Decide whether to associate RHS with LHS or the next node
        // look ahead to next token to see if it has higher value than this one
        int NextPrec = GetTokPrecedence();
        if (NextPrec < ExprPrec) {
            RHS = ParseBinOpRHS(TokPrec+1, RHS); // TokPrec is what is associated with the current operator. we want it higher than that
            // example: (1+2)*3*4*5. we recursive descend on 3 4 5
            if (RHS==0) return 0;
        }
        LHS = new BinaryExprAST(BinOp, LHS, RHS); // effectively we descend here.
    }
}

static PrototypeAST *ParsePrototype() {
    if (CurTok != tok_identifier) return ErrorP("Expected Function Name in Prototype");
    std::string FnName = IdentifierStr;
    getNextToken();
    if (CurTok!='(') return ErrorP("Expected (");
    std::vector<std::string> ArgNames;
    while(CurTok!=')') {
        ArgNames.push_back(IdentifierStr);
    }
    getNextToken(); // eat the ')'
    return new PrototypeAST(FnName, ArgNames);
}

static FunctionAST *ParseDefinition() {
    getNextToken();
    PrototypeAST *Proto = ParsePrototype();
    if (Proto==0) return 0; // i can haz exception? 
    if (ExprAST *E = ParseExpression()) return new FunctionAST(Proto, E);
    return 0;
}

static PrototypeAST *ParseExtern() {
    getNextToken();
    PrototypeAST *Proto = ParsePrototype();
    return Proto;
}

static FunctionAST *ParseTopLevelExpr() {
    if (ExprAST *E = ParseExpression()) {
        PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>()); //dummy proto
        return new FunctionAST(Proto, E);
    }
    return 0;
}

static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void MainLoop() {
    while(1) {
        fprintf(stderr, "ready >");
        switch(CurTok) {
        case tok_eof: return;
        case ';': getNextToken(); break;
        case tok_def: HandleDefinition(); break;
        case tok_extern: HandleExtern(); break;
        default:HandleTopLevelExpression();break;
        }
    }
}


int main() {
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}

