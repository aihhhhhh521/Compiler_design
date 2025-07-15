#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <stdexcept>

class CodeGenContext;

class Node {
public:
    virtual ~Node() = default;
    virtual void preprocess(CodeGenContext& context) { /* Do nothing by default */ }
    virtual void codegen(CodeGenContext& context) = 0;
};

class Expression : public Node {};

class Integer : public Expression {
public:
    int value;
    Integer(int val) : value(val) {}
    void codegen(CodeGenContext& context) override;
};

class Identifier : public Expression {
public:
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    void codegen(CodeGenContext& context) override;
};

class BinaryOperator : public Expression {
public:
    std::string op;
    Expression* lhs;
    Expression* rhs;
    BinaryOperator(Expression* l, const std::string& o, Expression* r) : op(o), lhs(l), rhs(r) {}
    void codegen(CodeGenContext& context) override;
};

class UnaryOperator : public Expression {
public:
    std::string op;
    Expression* rhs;
    UnaryOperator(const std::string& o, Expression* r) : op(o), rhs(r) {}
    void codegen(CodeGenContext& context) override;
};

class FunctionCall : public Expression {
public:
    std::string name;
    std::vector<Expression*> arguments;
    FunctionCall(const std::string& n, std::vector<Expression*> args) : name(n), arguments(std::move(args)) {}
    FunctionCall(const std::string& n, Expression* arg) : name(n) {
        if (arg) arguments.push_back(arg);
    }
    void codegen(CodeGenContext& context) override;
};

class Statement : public Node {};

class ExecutableStatement : public Statement {};

class Block : public Statement {
public:
    std::vector<Statement*> statements;
    void preprocess(CodeGenContext& context) override;
    void codegen(CodeGenContext& context) override;
};

class VariableDeclaration : public Statement {
public:
    std::vector<std::pair<std::string, Expression*>> declarations;

    VariableDeclaration(const std::string& name, Expression* init = nullptr) {
        declarations.push_back({name, init});
    }
    void preprocess(CodeGenContext& context) override;
    void codegen(CodeGenContext& context) override;
};

class ExpressionStatement : public Statement {
public:
    Expression* expression;
    ExpressionStatement(Expression* expr) : expression(expr) {}
    void codegen(CodeGenContext& context) override;
};

class ReturnStatement : public Statement {
public:
    Expression* expression;
    ReturnStatement(Expression* expr) : expression(expr) {}
    void codegen(CodeGenContext& context) override;
};

class IfStatement : public Statement {
public:
    Expression* condition;
    Statement* then_block;
    Statement* else_block;
    IfStatement(Expression* c, Statement* t, Statement* e) : condition(c), then_block(t), else_block(e) {}
    void preprocess(CodeGenContext& context) override;
    void codegen(CodeGenContext& context) override;
};

class WhileStatement : public Statement {
public:
    Expression* condition;
    Statement* body;
    WhileStatement(Expression* c, Statement* b) : condition(c), body(b) {}
    void preprocess(CodeGenContext& context) override;
    void codegen(CodeGenContext& context) override;
};

class BreakStatement : public Statement {
public:
    void codegen(CodeGenContext& context) override;
};

class ContinueStatement : public Statement {
public:
    void codegen(CodeGenContext& context) override;
};

class FunctionDefinition : public Node {
public:
    std::string type;
    std::string name;
    std::vector<VariableDeclaration*> params; // 函数参数
    Block* body;

    FunctionDefinition(const std::string& t, const std::string& n, std::vector<VariableDeclaration*> p, Block* b)
        : type(t), name(n), params(std::move(p)), body(b) {}
    
    void preprocess(CodeGenContext& context) override;
    void codegen(CodeGenContext& context) override;
};

class Program : public Node {
public:
    std::vector<FunctionDefinition*> functions;

    void preprocess(CodeGenContext& context) override {
        for (auto func : functions) {
            func->preprocess(context);
        }
    }
    void codegen(CodeGenContext& context) override {
        for (auto func : functions) {
            func->codegen(context);
        }
    }
};
