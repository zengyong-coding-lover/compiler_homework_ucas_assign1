#include "ASTInterpreter.h"
int main(int argc, char **argv) {
    if (argc > 1) {
        // llvm::errs() << argv[1];
        clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), argv[1]);
    }
}