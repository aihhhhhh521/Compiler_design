#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <sstream>

enum class TokenType {
    T_EOF, T_ILLEGAL,
    T_INT, T_RETURN, T_MAIN,
    T_IDENT, T_LITERAL,
    T_ASSIGN, T_SEMICOLON, T_COMMA,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_LT, T_LE, T_GT, T_GE, T_EQ, T_NE,
    T_BIT_AND, T_BIT_OR, T_BIT_XOR, T_LOGICAL_AND
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
    std::string source;
    size_t pos = 0;
    char currentChar;

public:
    Lexer(std::string src) : source(std::move(src)) {
        advance();
    }

    void advance() {
        if (pos >= source.length()) {
            currentChar = '\0';
        } else {
            currentChar = source[pos++];
        }
    }

    Token getNextToken() {
        while (isspace(currentChar)) advance();
        if (isalpha(currentChar) || currentChar == '_') return makeIdentifier();
        if (isdigit(currentChar)) return makeNumber();
        switch (currentChar) {
            case '\0': return {TokenType::T_EOF, ""};
            case '=':
                if (peek() == '=') { advance(); advance(); return {TokenType::T_EQ, "=="}; }
                advance(); return {TokenType::T_ASSIGN, "="};
            case ';': advance(); return {TokenType::T_SEMICOLON, ";"};
            case ',': advance(); return {TokenType::T_COMMA, ","};
            case '(': advance(); return {TokenType::T_LPAREN, "("};
            case ')': advance(); return {TokenType::T_RPAREN, ")"};
            case '{': advance(); return {TokenType::T_LBRACE, "{"};
            case '}': advance(); return {TokenType::T_RBRACE, "}"};
            case '+': advance(); return {TokenType::T_PLUS, "+"};
            case '-': advance(); return {TokenType::T_MINUS, "-"};
            case '*': advance(); return {TokenType::T_STAR, "*"};
            case '/': advance(); return {TokenType::T_SLASH, "/"};
            case '%': advance(); return {TokenType::T_PERCENT, "%"};
            case '<':
                if (peek() == '=') { advance(); advance(); return {TokenType::T_LE, "<="}; }
                advance(); return {TokenType::T_LT, "<"};
            case '>':
                if (peek() == '=') { advance(); advance(); return {TokenType::T_GE, ">="}; }
                advance(); return {TokenType::T_GT, ">"};
            case '!':
                if (peek() == '=') { advance(); advance(); return {TokenType::T_NE, "!="}; }
                break;
            case '&':
                if (peek() == '&') { advance(); advance(); return {TokenType::T_LOGICAL_AND, "&&"}; }
                advance(); return {TokenType::T_BIT_AND, "&"};
            case '|': advance(); return {TokenType::T_BIT_OR, "|"};
            case '^': advance(); return {TokenType::T_BIT_XOR, "^"};
        }
        return {TokenType::T_ILLEGAL, std::string(1, currentChar)};
    }
private:
    char peek() { return (pos < source.length()) ? source[pos] : '\0'; }
    Token makeIdentifier() {
        std::string ident;
        while (isalnum(currentChar) || currentChar == '_') {
            ident += currentChar;
            advance();
        }
        if (ident == "int") return {TokenType::T_INT, "int"};
        if (ident == "return") return {TokenType::T_RETURN, "return"};
        if (ident == "main") return {TokenType::T_MAIN, "main"};
        if (ident == "println_int") return {TokenType::T_IDENT, "println_int"};
        return {TokenType::T_IDENT, ident};
    }
    Token makeNumber() {
        std::string num;
        while (isdigit(currentChar)) {
            num += currentChar;
            advance();
        }
        return {TokenType::T_LITERAL, num};
    }
};

class Compiler {
    Lexer lexer;
    Token currentToken;
    std::map<std::string, int> localVars;
    std::vector<std::string> globalVars;
    std::vector<std::string> code;
    int stackIndex = 0;
    bool hasMainReturn = false;
    int labelCounter = 0;

public:
    Compiler(const std::string& source) : lexer(source) {
        currentToken = lexer.getNextToken();
    }
    
    void compile() {
        parseProgram(); 
        emitAllSections(); 
    }

private:
    void eat(TokenType type) {
        if (currentToken.type == type) {
            currentToken = lexer.getNextToken();
        } else {
            throw std::runtime_error("预料之外的词法单元: " + currentToken.value);
        }
    }

    void emit(const std::string& s) { code.push_back("\t" + s); }
    void emitLabel(const std::string& s) { code.push_back(s + ":"); }
    
    bool isGlobal(const std::string& name) {
        for(const auto& var : globalVars) if (var == name) return true;
        return false;
    }
    
    bool isLocal(const std::string& name) {
        return localVars.count(name) > 0;
    }

    void parseProgram() {
        while(currentToken.type != TokenType::T_EOF) {
            parseTopLevelStatement();
        }
    }

    void emitAllSections() {
        std::cout << ".intel_syntax noprefix" << std::endl;
        std::cout << ".global main" << std::endl;
        std::cout << ".extern printf" << std::endl;
        
        std::cout << ".data" << std::endl;
        std::cout << "format_str:\t.asciz \"%d\\n\"" << std::endl;
        for(const auto& var : globalVars) {
            std::cout << var << ":\t.long 0" << std::endl;
        }
        
        std::cout << ".text" << std::endl;
        for(const auto& line : code) {
            std::cout << line << std::endl;
        }
    }

    void parseTopLevelStatement() {
        eat(TokenType::T_INT);
        if (currentToken.type == TokenType::T_MAIN) {
            parseMain();
        } else { 
            globalVars.push_back(currentToken.value);
            eat(TokenType::T_IDENT);
            eat(TokenType::T_SEMICOLON);
        }
    }

    void parseMain() {
        eat(TokenType::T_MAIN);
        emitLabel("main");
        emit("push ebp");
        emit("mov ebp, esp");
        emit("sub esp, 256"); 

        eat(TokenType::T_LPAREN);
        if (currentToken.type == TokenType::T_INT) {
            eat(TokenType::T_INT);
            eat(TokenType::T_IDENT); 
            if (currentToken.type == TokenType::T_COMMA) {
                eat(TokenType::T_COMMA); 
                eat(TokenType::T_INT);
                eat(TokenType::T_IDENT); 
            }
        }
        eat(TokenType::T_RPAREN);
        parseBlock();

        if (!hasMainReturn) {
            emit("mov eax, 0"); 
            emit("leave");
            emit("ret");
        }
    }

    void parseBlock() {
        eat(TokenType::T_LBRACE);
        while (currentToken.type != TokenType::T_RBRACE && currentToken.type != TokenType::T_EOF) {
            parseStatement();
        }
        eat(TokenType::T_RBRACE);
    }
    
    void parseStatement() {
        switch (currentToken.type) {
            case TokenType::T_INT:      parseDeclaration(); break;
            case TokenType::T_RETURN:   parseReturn(); hasMainReturn = true; break;
            case TokenType::T_IDENT:
                if (currentToken.value == "println_int") {
                    parsePrintln();
                } else {
                    parseAssignment();
                }
                break;
            default:
                throw std::runtime_error("无效的语句开头: " + currentToken.value);
        }
    }
    
    void parseDeclaration() {
        eat(TokenType::T_INT);
        stackIndex -= 4;
        localVars[currentToken.value] = stackIndex;
        eat(TokenType::T_IDENT);
        eat(TokenType::T_SEMICOLON);
    }

    void parseReturn() {
        eat(TokenType::T_RETURN);
        parseExpression();
        eat(TokenType::T_SEMICOLON);
        emit("pop eax");
        emit("leave");
        emit("ret");
    }
    
    void parseAssignment() {
        std::string varName = currentToken.value;
        eat(TokenType::T_IDENT);
        eat(TokenType::T_ASSIGN);
        parseExpression();
        eat(TokenType::T_SEMICOLON);
        emit("pop eax");
        
        if (isLocal(varName)) {
            emit("mov DWORD PTR [ebp" + std::to_string(localVars[varName]) + "], eax");
        } else if (isGlobal(varName)) {
            emit("mov DWORD PTR [" + varName + "], eax");
        } else {
            throw std::runtime_error("未声明的变量: " + varName);
        }
    }

    void parsePrintln() {
        eat(TokenType::T_IDENT); 
        eat(TokenType::T_LPAREN);
        parseExpression();
        eat(TokenType::T_RPAREN);
        eat(TokenType::T_SEMICOLON);
        
        emit("push DWORD PTR [esp]");
        emit("push offset format_str");
        emit("call printf");
        emit("add esp, 8");
        emit("pop eax"); 
    }

    void parseExpression() {
        parseLogicalAndExpr();
    }

    void parseLogicalAndExpr() {
        parseBitwiseOrExpr();
        while (currentToken.type == TokenType::T_LOGICAL_AND) {
            eat(TokenType::T_LOGICAL_AND);
            std::string falseLabel = "_L_false" + std::to_string(labelCounter);
            std::string endLabel = "_L_end" + std::to_string(labelCounter++);

            emit("pop eax");
            emit("cmp eax, 0");
            emit("je " + falseLabel);

            parseBitwiseOrExpr();
            emit("pop eax");
            emit("cmp eax, 0");
            emit("je " + falseLabel); 

            emit("mov eax, 1"); 
            emit("jmp " + endLabel);

            emitLabel(falseLabel);
            emit("mov eax, 0"); 

            emitLabel(endLabel);
            emit("push eax");
        }
    }

    void parseBitwiseOrExpr() {
        parseBitwiseXorExpr();
        while(currentToken.type == TokenType::T_BIT_OR) {
            eat(TokenType::T_BIT_OR);
            parseBitwiseXorExpr();
            emit("pop edx");
            emit("pop eax");
            emit("or eax, edx");
            emit("push eax");
        }
    }

    void parseBitwiseXorExpr() {
        parseBitwiseAndExpr();
        while(currentToken.type == TokenType::T_BIT_XOR) {
            eat(TokenType::T_BIT_XOR);
            parseBitwiseAndExpr();
            emit("pop edx");
            emit("pop eax");
            emit("xor eax, edx");
            emit("push eax");
        }
    }

    void parseBitwiseAndExpr() {
        parseEqualityExpr();
        while(currentToken.type == TokenType::T_BIT_AND) {
            eat(TokenType::T_BIT_AND);
            parseEqualityExpr();
            emit("pop edx");
            emit("pop eax");
            emit("and eax, edx");
            emit("push eax");
        }
    }

    void parseEqualityExpr() {
        parseRelationalExpr();
        while (currentToken.type == TokenType::T_EQ || currentToken.type == TokenType::T_NE) {
            Token op = currentToken;
            eat(op.type);
            parseRelationalExpr();
            emit("pop edx");
            emit("pop eax");
            emit("cmp eax, edx");
            if (op.type == TokenType::T_EQ) emit("sete al");
            else emit("setne al");
            emit("movzx eax, al");
            emit("push eax");
        }
    }

    void parseRelationalExpr() {
        parseAdditiveExpr();
        while (currentToken.type == TokenType::T_LT || currentToken.type == TokenType::T_LE ||
               currentToken.type == TokenType::T_GT || currentToken.type == TokenType::T_GE) {
            Token op = currentToken;
            eat(op.type);
            parseAdditiveExpr();
            emit("pop edx");
            emit("pop eax");
            emit("cmp eax, edx");
            if (op.type == TokenType::T_LT) emit("setl al");
            else if (op.type == TokenType::T_LE) emit("setle al");
            else if (op.type == TokenType::T_GT) emit("setg al");
            else emit("setge al");
            emit("movzx eax, al");
            emit("push eax");
        }
    }

    void parseAdditiveExpr() {
        parseMultiplicativeExpr();
        while (currentToken.type == TokenType::T_PLUS || currentToken.type == TokenType::T_MINUS) {
            Token op = currentToken;
            eat(op.type);
            parseMultiplicativeExpr();
            emit("pop edx");
            emit("pop eax");
            if (op.type == TokenType::T_PLUS) emit("add eax, edx");
            else emit("sub eax, edx");
            emit("push eax");
        }
    }

    void parseMultiplicativeExpr() {
        parseUnaryExpr();
        while (currentToken.type == TokenType::T_STAR || currentToken.type == TokenType::T_SLASH || currentToken.type == TokenType::T_PERCENT) {
            Token op = currentToken;
            eat(op.type);
            parseUnaryExpr();
            if (op.type == TokenType::T_STAR) {
                emit("pop edx");
                emit("pop eax");
                emit("imul eax, edx");
            } else { 
                emit("pop ecx"); 
                emit("pop eax"); 
                emit("cdq");     
                emit("idiv ecx");
                if (op.type == TokenType::T_PERCENT) {
                    emit("mov eax, edx"); 
                }
            }
            emit("push eax");
        }
    }
    
    void parseUnaryExpr() {
        if(currentToken.type == TokenType::T_MINUS) {
            eat(TokenType::T_MINUS);
            parseUnaryExpr();
            emit("pop eax");
            emit("neg eax");
            emit("push eax");
        } else {
            parsePrimaryExpr();
        }
    }

    void parsePrimaryExpr() {
        if (currentToken.type == TokenType::T_LITERAL) {
            emit("push " + currentToken.value);
            eat(TokenType::T_LITERAL);
        } else if (currentToken.type == TokenType::T_IDENT) {
            std::string varName = currentToken.value;
            if (isLocal(varName)) {
                emit("push DWORD PTR [ebp" + std::to_string(localVars[varName]) + "]");
            } else if (isGlobal(varName)) {
                emit("push DWORD PTR [" + varName + "]");
            } else {
                throw std::runtime_error("未声明的变量: " + varName);
            }
            eat(TokenType::T_IDENT);
        } else if (currentToken.type == TokenType::T_LPAREN) {
            eat(TokenType::T_LPAREN);
            parseExpression(); 
            eat(TokenType::T_RPAREN);
        } else {
            throw std::runtime_error("无效的主表达式: " + currentToken.value);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "用法: " << argv[0] << " <输入文件>" << std::endl;
        return 1;
    }
    std::ifstream inputFile(argv[1]);
    if (!inputFile) {
        std::cerr << "错误: 无法打开文件 " << argv[1] << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string source = buffer.str();

    try {
        Compiler compiler(source);
        compiler.compile();
    } catch (const std::runtime_error& e) {
        std::cerr << "编译错误: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}