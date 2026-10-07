
#include "Node.h"

#include <string>

#include <karpuz/Compiler.h>

int64_t Node::s_next_id;
std::vector<Node::Ptr> Node::s_roots;
Token::Ptr curtoken;

Node::Node() : m_id(FF("Ki{}", ++s_next_id)) {}

Node::~Node() {}

Node::Ptr Node::compute_stmt_type(SymbolTable &st) {
    set_cur_symtab(st.get_cur_symtab());
    return nullptr;
}

Node::Ptr Node::add_to_symtab_forward(SymbolTable &st) {
    return nullptr;
}

Node::Ptr Node::add_to_symtab_ordered(SymbolTable &st) {
    return nullptr;
}

Node::Ptr Node::pop_root() {
    assert(! s_roots.empty());
    auto retval = s_roots.back();
    s_roots.pop_back();
    return retval;
}

const Node::Ptr &Node::get_root_before() {
    assert(s_roots.size() > 1);
    return *std::next(s_roots.rbegin());
}

Node::Ptr Node::gen_wat(WasmContext &) {
    assert(! m_id.empty());
    return nullptr;
}

Node::Ptr Node::gen_wat(WasmContext &, const std::string &id) const {
    assert(! id.empty());
    return nullptr;
}
