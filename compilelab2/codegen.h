#pragma once

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <stdexcept>

struct FunctionInfo {
    int arg_count;
};

class CodeGenContext {
public:
    std::map<std::string, FunctionInfo> function_table;
    std::stack<std::map<std::string, int>> symbol_table_stack;
    int current_offset = 0;
    std::stack<std::pair<int, int>> loop_labels;

    void push_scope();
    void pop_scope();
    void add_variable(const std::string& name);
    int get_variable_offset(const std::string& name);
    
    int get_stack_size() const;
    void push_loop_labels(int continue_label, int break_label);
    void pop_loop_labels();
    std::pair<int, int> get_current_loop_labels();
};