
#include "Compiler.h"
#include <cassert>

#include <fmt/format.h>

#include <resource/FILE_io_ki.h>

Node::Ptr SymbolTable::s_module_ki;
Node::Ptr SymbolTable::s_module_io;

SymbolTable::~SymbolTable() {}

Compiler *Compiler::s_current;

extern Token::Ptr curtoken;

Compiler::Compiler() {
    assert(! s_current);
    s_current = this;
    Node::reset_root();
}

Compiler::~Compiler() {
    if (buffer) {
        yy_delete_buffer(buffer);
        buffer = nullptr;
    }
    reset_parser();
    s_current = nullptr;
}

int Compiler::compile_file(const std::string &file_name) {
    yyin = fopen(file_name.data(), "rb");
    if (! yyin) {
        perror(file_name.data());
        return 2;
    }

    yyparse();
    auto root = Node::get_root();
    reset();

    return compile(root);
}

int Compiler::compile_string(const std::string &code) {
    buffer = yy_scan_string(code.data());
    yyparse();
    auto root = Node::get_root();
    reset();

    return compile(root);
}

Node::Ptr Compiler::compile_module(const std::string &str) {
    buffer = yy_scan_string(str.data());
    yyparse();
    auto retval = Node::pop_root();
    reset();
    assert(retval);
    return retval;
}

void Compiler::reset_parser() {
    curtoken.reset();
    Node::reset_root();
    Token::colno = 0;
    yylex_destroy();
}

void Compiler::reset() {
    if (buffer) {
        yy_delete_buffer(buffer);
        buffer = nullptr;
    }
    reset_parser();
}

int Compiler::compile(Node::Ptr root) {
    if (! root) {
        return 1;
    }

    SymbolTable st(ScopeType::Module);

    if (auto ret = root->compute_stmt_type(st)) {
        set_error(FF("Error at {}:{}: {}\n", ret->get_line(), ret->get_col(), ret->get_error()));
        Node::reset_root();
        return 1;
    }

    if (auto ret = root->gen_wat(m_ctx)) {
        return 2;
    }

    return 0;
}

SymbolTable::SymbolTable()
        : m_symbols({
                  std::make_shared<Scope>(Scope::SymTab{}, ScopeType::Module, nullptr),
          }) {
    if (! s_module_io) {
        s_module_io = Compiler::current()->compile_module(FILE_io_ki);
    }
}

SymbolTable::SymbolTable(ScopeType scope_type) : SymbolTable() {
    m_symbols.back()->scope_type = scope_type;
}

WasmContext::Coords WasmContext::add_to_memory(const std::string &s) {
    assert(! s.empty());
    return {}; // TODO:
}

WasmContext::Coords WasmContext::add_to_memory(uint32_t s) {
    return {}; // TODO:
}
