#include "codegen.h"
#include "node.h"
#include <iostream>
#include <stdexcept>

extern int label_count = 0;

void CodeGenContext::push_scope() {
    symbol_table_stack.push({});
    current_offset = 0;
}

void CodeGenContext::pop_scope() {
    if (!symbol_table_stack.empty()) {
        symbol_table_stack.pop();
    }
}

void CodeGenContext::add_variable(const std::string& name) {
    if (symbol_table_stack.top().count(name)) {
        throw std::runtime_error("Error: Redeclaration of variable '" + name + "'");
    }
    current_offset += 4;
    symbol_table_stack.top()[name] = current_offset;
}

int CodeGenContext::get_variable_offset(const std::string& name) {
    auto& current_scope = symbol_table_stack.top();
    if (current_scope.find(name) == current_scope.end()) {
        throw std::runtime_error("Error: Undeclared variable '" + name + "'");
    }
    return current_scope[name];
}

int CodeGenContext::get_stack_size() const {
    return (current_offset + 15) & ~15;
}

void CodeGenContext::push_loop_labels(int continue_label, int break_label) {
    loop_labels.push({continue_label, break_label});
}

void CodeGenContext::pop_loop_labels() {
    if (!loop_labels.empty()) {
        loop_labels.pop();
    }
}

std::pair<int, int> CodeGenContext::get_current_loop_labels() {
    if (loop_labels.empty()) {
        throw std::runtime_error("Error: 'break' or 'continue' used outside of a loop");
    }
    return loop_labels.top();
}

void Block::preprocess(CodeGenContext& context) {
    for (auto stmt : statements) {
        stmt->preprocess(context);
    }
}

void VariableDeclaration::preprocess(CodeGenContext& context) {
    for (const auto& decl : declarations) {
        context.add_variable(decl.first);
    }
}

void Integer::codegen(CodeGenContext& context) {
    std::cout << "  push " << value << std::endl;
}

void Identifier::codegen(CodeGenContext& context) {
    int offset = context.get_variable_offset(name);
    std::cout << "  mov eax, DWORD PTR [ebp-" << offset << "]" << std::endl;
    std::cout << "  push eax" << std::endl;
}

void UnaryOperator::codegen(CodeGenContext& context) {
    rhs->codegen(context);
    std::cout << "  pop eax" << std::endl;
    if (op == "-") {
        std::cout << "  neg eax" << std::endl;
    } else if (op == "~") {
        std::cout << "  not eax" << std::endl;
    } else if (op == "!") {
        std::cout << "  cmp eax, 0" << std::endl;
        std::cout << "  sete al" << std::endl;
        std::cout << "  movzx eax, al" << std::endl;
    }
    std::cout << "  push eax" << std::endl;
}

void BinaryOperator::codegen(CodeGenContext& context) {
    if (op == "=") {
        Identifier* id = dynamic_cast<Identifier*>(lhs);
        if (!id) throw std::runtime_error("Error: LHS of assignment must be a variable.");
        
        int offset = context.get_variable_offset(id->name);
        rhs->codegen(context);
        std::cout << "  pop eax" << std::endl;
        std::cout << "  mov DWORD PTR [ebp-" << offset << "], eax" << std::endl;
        std::cout << "  push eax" << std::endl;
        return;
    }

    rhs->codegen(context);
    lhs->codegen(context);

    std::cout << "  pop eax" << std::endl;
    std::cout << "  pop ebx" << std::endl;
    
    if (op == "+") std::cout << "  add eax, ebx" << std::endl;
    else if (op == "-") std::cout << "  sub eax, ebx" << std::endl;
    else if (op == "*") std::cout << "  imul eax, ebx" << std::endl;
    else if (op == "/" || op == "%") {
        std::cout << "  cdq" << std::endl;
        std::cout << "  idiv ebx" << std::endl;
        if (op == "%") std::cout << "  mov eax, edx" << std::endl;
    } 
    else if (op == "&&") {
        std::cout << "  and eax, ebx" << std::endl;
        std::cout << "  cmp eax, 0" << std::endl;
        std::cout << "  setne al" << std::endl;
        std::cout << "  movzx eax, al" << std::endl;
    }
    else if (op == "||") {
        std::cout << "  or eax, ebx" << std::endl;
        std::cout << "  cmp eax, 0" << std::endl;
        std::cout << "  setne al" << std::endl;
        std::cout << "  movzx eax, al" << std::endl;
    }
    else if (op == "&")  std::cout << "  and eax, ebx" << std::endl;
    else if (op == "|")  std::cout << "  or eax, ebx" << std::endl;
    else if (op == "^")  std::cout << "  xor eax, ebx" << std::endl;
    else {
        std::cout << "  cmp eax, ebx" << std::endl;
        if (op == "==") std::cout << "  sete al" << std::endl;
        else if (op == "!=") std::cout << "  setne al" << std::endl;
        else if (op == "<") std::cout << "  setl al" << std::endl;
        else if (op == "<=") std::cout << "  setle al" << std::endl;
        else if (op == ">") std::cout << "  setg al" << std::endl;
        else if (op == ">=") std::cout << "  setge al" << std::endl;
        std::cout << "  movzx eax, al" << std::endl;
    }
    
    std::cout << "  push eax" << std::endl;
}

void FunctionCall::codegen(CodeGenContext& context) {
    if (name == "println_int") {
        if (arguments.size() != 1) throw std::runtime_error("Error: println_int expects 1 argument");
        arguments[0]->codegen(context);
        std::cout << "  push offset format_str" << std::endl;
        std::cout << "  call printf" << std::endl;
        std::cout << "  add esp, 8" << std::endl; 
        std::cout << "  push 0" << std::endl; 
    } else {
        for (auto it = arguments.rbegin(); it != arguments.rend(); ++it) {
            (*it)->codegen(context);
        }
        std::cout << "  call " << name << std::endl;
        if (!arguments.empty()) {
            std::cout << "  add esp, " << arguments.size() * 4 << std::endl;
        }
        std::cout << "  push eax" << std::endl;
    }
}

void Block::codegen(CodeGenContext& context) {
    for (auto stmt : statements) {
        stmt->codegen(context);
    }
}

void VariableDeclaration::codegen(CodeGenContext& context) {
    for (const auto& decl : declarations) {
        if (decl.second) {
            int offset = context.get_variable_offset(decl.first);
            decl.second->codegen(context);
            std::cout << "  pop eax" << std::endl;
            std::cout << "  mov DWORD PTR [ebp-" << offset << "], eax" << std::endl;
        }
    }
}

void ExpressionStatement::codegen(CodeGenContext& context) {
    FunctionCall* func_call = dynamic_cast<FunctionCall*>(expression);
    bool is_void_call = (func_call && func_call->name == "println_int");

    expression->codegen(context);

    if (!is_void_call) {
        std::cout << "  add esp, 4" << std::endl;
    }
}

void ReturnStatement::codegen(CodeGenContext& context) {
    expression->codegen(context);
    std::cout << "  pop eax" << std::endl; 
    std::cout << "  leave" << std::endl;     
    std::cout << "  ret" << std::endl;       
}

void IfStatement::codegen(CodeGenContext& context) {
    int else_label = label_count++;
    int end_label = label_count++;

    condition->codegen(context);
    std::cout << "  pop eax" << std::endl;
    std::cout << "  cmp eax, 0" << std::endl;

    if (else_block) {
        std::cout << "  je .L_else_" << else_label << std::endl;
    } else {
        std::cout << "  je .L_end_" << end_label << std::endl;
    }

    then_block->codegen(context);
    if (else_block) {
        std::cout << "  jmp .L_end_" << end_label << std::endl;
        std::cout << ".L_else_" << else_label << ":" << std::endl;
        else_block->codegen(context);
    }

    std::cout << ".L_end_" << end_label << ":" << std::endl;
}

void WhileStatement::codegen(CodeGenContext& context) {
    int cond_label = label_count++;
    int end_label = label_count++;
    
    context.push_loop_labels(cond_label, end_label);

    std::cout << ".L_cond_" << cond_label << ":" << std::endl;
    condition->codegen(context);
    std::cout << "  pop eax" << std::endl;
    std::cout << "  cmp eax, 0" << std::endl;
    std::cout << "  je .L_end_" << end_label << std::endl;
    
    body->codegen(context);
    
    std::cout << "  jmp .L_cond_" << cond_label << std::endl;
    std::cout << ".L_end_" << end_label << ":" << std::endl;

    context.pop_loop_labels();
}

void BreakStatement::codegen(CodeGenContext& context) {
    auto labels = context.get_current_loop_labels();
    std::cout << "  jmp .L_end_" << labels.second << std::endl;
}

void ContinueStatement::codegen(CodeGenContext& context) {
    auto labels = context.get_current_loop_labels();
    std::cout << "  jmp .L_cond_" << labels.first << std::endl;
}

void IfStatement::preprocess(CodeGenContext& context) {
    if (then_block) {
        then_block->preprocess(context);
    }
    if (else_block) {
        else_block->preprocess(context);
    }
}

void WhileStatement::preprocess(CodeGenContext& context) {
    if (body) {
        body->preprocess(context);
    }
}

void FunctionDefinition::preprocess(CodeGenContext& context) {
    context.function_table[name] = { (int)params.size() };
    context.push_scope(); 
    
    int param_offset = 8; 
    for (auto param : params) {
        for (const auto& p_decl : param->declarations) {
            context.symbol_table_stack.top()[p_decl.first] = -param_offset; 
            param_offset += 4;
        }
    }
    
    body->preprocess(context);

    context.pop_scope(); 
}

void FunctionDefinition::codegen(CodeGenContext& context) {
    std::cout << ".global " << name << std::endl;
    std::cout << name << ":" << std::endl;

    std::cout << "  push ebp" << std::endl;
    std::cout << "  mov ebp, esp" << std::endl;

    context.push_scope();
    int param_offset = 8;
    for (auto param : params) {
         for (const auto& p_decl : param->declarations) {
            context.symbol_table_stack.top()[p_decl.first] = -param_offset;
            param_offset += 4;
        }
    }
    body->preprocess(context); 
    int stack_size = context.get_stack_size();
    if (stack_size > 0) {
        std::cout << "  sub esp, " << stack_size << std::endl;
    }

    body->codegen(context);

    if (type == "void") {
        std::cout << "  leave" << std::endl;
        std::cout << "  ret" << std::endl;
    }

    context.pop_scope();
}