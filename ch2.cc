#import <string>
#import <vector>


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
