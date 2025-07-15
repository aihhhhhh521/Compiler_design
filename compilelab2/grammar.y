%{
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include "node.h"

extern int yylex();
extern int yylineno;
extern char* yytext;

void yyerror(const char *s);

Program* program_root = nullptr;
%}

%union {
    Node* node;
    Program* program;
    FunctionDefinition* func_def;
    Block* block;
    Statement* stmt;
    Expression* expr;
    VariableDeclaration* var_decl;
    std::vector<FunctionDefinition*>* func_defs;
    std::vector<Statement*>* stmts;
    std::vector<Expression*>* exprs;
    std::vector<VariableDeclaration*>* params;
    std::vector<std::pair<std::string, Expression*>>* var_inits;
    std::string* str;
    int                 integer;
}

%token <str> IDENTIFIER
%token <integer> INTEGER_LITERAL
%token VOID INT RETURN IF ELSE WHILE BREAK CONTINUE
%token SEMI COMMA LPAREN RPAREN LBRACE RBRACE
%token ASSIGN
%token OR_OP AND_OP
%token EQ_OP NE_OP LT_OP LE_OP GT_OP GE_OP
%token PLUS MINUS STAR SLASH PERCENT
%token BIT_OR BIT_XOR BIT_AND
%token NOT LOG_NOT BIT_NOT

%type<program>    program_rule
%type<func_defs>  function_list
%type<func_def>   function_definition
%type<str>        type_specifier
%type<params>     param_list param_declarations
%type<var_decl>   param_declaration
%type<block>      compound_statement
%type<stmts>      statement_list
%type<stmt>       statement declaration_statement expression_statement selection_statement iteration_statement jump_statement
%type<var_inits>  var_init_list
%type<expr>       expression assignment_expression logical_or_expression logical_and_expression inclusive_or_expression
%type<expr>       exclusive_or_expression and_expression equality_expression relational_expression
%type<expr>       additive_expression multiplicative_expression unary_expression postfix_expression primary_expression
%type<exprs>      argument_list

%right ASSIGN
%left  OR_OP
%left  AND_OP
%left  BIT_OR
%left  BIT_XOR
%left  BIT_AND
%left  EQ_OP NE_OP
%left  LT_OP LE_OP GT_OP GE_OP
%left  PLUS MINUS
%left  STAR SLASH PERCENT
%right LOG_NOT BIT_NOT UNARY_MINUS
%nonassoc "then"
%nonassoc ELSE

%%

program_rule:
    function_list
    {
        program_root = new Program();
        program_root->functions = *$1;
        delete $1;
    }
    ;

function_list:
    function_definition
    {
        $$ = new std::vector<FunctionDefinition*>();
        $$->push_back($1);
    }
    | function_list function_definition
    {
        $1->push_back($2);
        $$ = $1;
    }
    ;

function_definition:
    type_specifier IDENTIFIER LPAREN param_list RPAREN compound_statement
    {
        $$ = new FunctionDefinition(*$1, *$2, *$4, $6);
        delete $1;
        delete $2;
        delete $4;
    }
    ;

type_specifier:
    INT { $$ = new std::string("int"); }
    | VOID { $$ = new std::string("void"); }
    ;

param_list:
    {
        $$ = new std::vector<VariableDeclaration*>();
    }
    | param_declarations
    ;

param_declarations:
    param_declaration
    {
        $$ = new std::vector<VariableDeclaration*>();
        $$->push_back($1);
    }
    | param_declarations COMMA param_declaration
    {
        $1->push_back($3);
        $$ = $1;
    }
    ;

param_declaration:
    type_specifier IDENTIFIER
    {
        $$ = new VariableDeclaration(*$2, nullptr);
        delete $1;
        delete $2;
    }
    ;

statement:
    expression_statement
    | compound_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | declaration_statement
    ;

compound_statement:
    LBRACE RBRACE
    {
        $$ = new Block();
    }
    | LBRACE statement_list RBRACE
    {
        $$ = new Block();
        $$->statements = *$2;
        delete $2;
    }
    ;

statement_list:
    statement
    {
        $$ = new std::vector<Statement*>();
        $$->push_back($1);
    }
    | statement_list statement
    {
        $1->push_back($2);
        $$ = $1;
    }
    ;

declaration_statement:
    type_specifier var_init_list SEMI
    {
        auto declNode = new VariableDeclaration("", nullptr);
        declNode->declarations.clear();
        declNode->declarations = *$2;
        $$ = declNode;
        delete $1;
        delete $2;
    }
    ;

var_init_list:
    IDENTIFIER
    {
        $$ = new std::vector<std::pair<std::string, Expression*>>();
        $$->push_back({*$1, nullptr});
        delete $1;
    }
    | IDENTIFIER ASSIGN assignment_expression
    {
        $$ = new std::vector<std::pair<std::string, Expression*>>();
        $$->push_back({*$1, $3});
        delete $1;
    }
    | var_init_list COMMA IDENTIFIER
    {
        $1->push_back({*$3, nullptr});
        $$ = $1;
        delete $3;
    }
    | var_init_list COMMA IDENTIFIER ASSIGN assignment_expression
    {
        $1->push_back({*$3, $5});
        $$ = $1;
        delete $3;
    }
    ;


expression_statement:
    SEMI { $$ = new Block(); }
    | expression SEMI { $$ = new ExpressionStatement($1); }
    ;

selection_statement:
    IF LPAREN expression RPAREN statement %prec "then"
    {
        $$ = new IfStatement($3, $5, nullptr);
    }
    | IF LPAREN expression RPAREN statement ELSE statement
    {
        $$ = new IfStatement($3, $5, $7);
    }
    ;

iteration_statement:
    WHILE LPAREN expression RPAREN statement
    {
        $$ = new WhileStatement($3, $5);
    }
    ;

jump_statement:
    RETURN SEMI
    {
        $$ = new ReturnStatement(new Integer(0));
    }
    | RETURN expression SEMI
    {
        $$ = new ReturnStatement($2);
    }
    | BREAK SEMI { $$ = new BreakStatement(); }
    | CONTINUE SEMI { $$ = new ContinueStatement(); }
    ;

expression:
    assignment_expression
    ;

assignment_expression:
    logical_or_expression
    | unary_expression ASSIGN assignment_expression
    {
        auto id = dynamic_cast<Identifier*>($1);
        if (!id) {
            yyerror("LHS of assignment must be an identifier");
        }
        $$ = new BinaryOperator(id, "=", $3);
    }
    ;

logical_or_expression:
    logical_and_expression
    | logical_or_expression OR_OP logical_and_expression { $$ = new BinaryOperator($1, "||", $3); }
    ;

logical_and_expression:
    inclusive_or_expression
    | logical_and_expression AND_OP inclusive_or_expression { $$ = new BinaryOperator($1, "&&", $3); }
    ;

inclusive_or_expression:
    exclusive_or_expression
    | inclusive_or_expression BIT_OR exclusive_or_expression { $$ = new BinaryOperator($1, "|", $3); }
    ;

exclusive_or_expression:
    and_expression
    | exclusive_or_expression BIT_XOR and_expression { $$ = new BinaryOperator($1, "^", $3); }
    ;

and_expression:
    equality_expression
    | and_expression BIT_AND equality_expression { $$ = new BinaryOperator($1, "&", $3); }
    ;

equality_expression:
    relational_expression
    | equality_expression EQ_OP relational_expression { $$ = new BinaryOperator($1, "==", $3); }
    | equality_expression NE_OP relational_expression { $$ = new BinaryOperator($1, "!=", $3); }
    ;

relational_expression:
    additive_expression
    | relational_expression LT_OP additive_expression { $$ = new BinaryOperator($1, "<", $3); }
    | relational_expression GT_OP additive_expression { $$ = new BinaryOperator($1, ">", $3); }
    | relational_expression LE_OP additive_expression { $$ = new BinaryOperator($1, "<=", $3); }
    | relational_expression GE_OP additive_expression { $$ = new BinaryOperator($1, ">=", $3); }
    ;

additive_expression:
    multiplicative_expression
    | additive_expression PLUS multiplicative_expression { $$ = new BinaryOperator($1, "+", $3); }
    | additive_expression MINUS multiplicative_expression { $$ = new BinaryOperator($1, "-", $3); }
    ;

multiplicative_expression:
    unary_expression
    | multiplicative_expression STAR unary_expression { $$ = new BinaryOperator($1, "*", $3); }
    | multiplicative_expression SLASH unary_expression { $$ = new BinaryOperator($1, "/", $3); }
    | multiplicative_expression PERCENT unary_expression { $$ = new BinaryOperator($1, "%", $3); }
    ;

unary_expression:
    postfix_expression
    | MINUS unary_expression %prec UNARY_MINUS { $$ = new UnaryOperator("-", $2); }
    | LOG_NOT unary_expression { $$ = new UnaryOperator("!", $2); }
    | BIT_NOT unary_expression { $$ = new UnaryOperator("~", $2); }
    ;

postfix_expression:
    primary_expression
    | IDENTIFIER LPAREN RPAREN
    {
        $$ = new FunctionCall(*$1, std::vector<Expression*>{});
        delete $1;
    }
    | IDENTIFIER LPAREN argument_list RPAREN
    {
        $$ = new FunctionCall(*$1, *$3);
        delete $1;
        delete $3;
    }
    ;

argument_list:
    assignment_expression
    {
        $$ = new std::vector<Expression*>();
        $$->push_back($1);
    }
    | argument_list COMMA assignment_expression
    {
        $1->push_back($3);
        $$ = $1;
    }
    ;

primary_expression:
    IDENTIFIER { $$ = new Identifier(*$1); delete $1; }
    | INTEGER_LITERAL { $$ = new Integer($1); }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error on line %d: %s near '%s'\n", yylineno, s, yytext);
    exit(1);
}