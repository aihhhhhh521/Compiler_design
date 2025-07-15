#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <stdexcept>

#include "node.h"
#include "codegen.h"


extern int yyparse();
extern FILE *yyin;
extern Program* program_root; 

int main(int argc, char **argv) {
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    yyparse();

    if (program_root) {
        try {
            CodeGenContext context;
            
            std::cout << ".intel_syntax noprefix" << std::endl;
            std::cout << std::endl;
            
            std::cout << ".data" << std::endl;
            std::cout << "format_str:" << std::endl;
            std::cout << "  .asciz \"%d\\n\"" << std::endl;
            std::cout << ".extern printf" << std::endl << std::endl;
            
            std::cout << ".text" << std::endl;
            
            program_root->preprocess(context);
            program_root->codegen(context);

        } catch (const std::runtime_error& e) {
            std::cerr << "An error occurred during compilation: " << e.what() << std::endl;
            fclose(yyin);
            return 1;
        }
    }

    fclose(yyin);
    return 0;
}