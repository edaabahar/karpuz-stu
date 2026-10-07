#ifndef KARPUZ_NODE_H
#define KARPUZ_NODE_H

#include <cassert>
#include <sstream>
#include <vector>

#include <fmt/ranges.h>
#define FF fmt::format
#include <fmt/format.h>

#include <karpuz/Token.h>

extern int yylineno;

class SymbolTable;
struct Scope;
class WasmContext;
class Node : public std::enable_shared_from_this<Node> {
public:
    using Ptr = std::shared_ptr<Node>;
    using Cptr = std::shared_ptr<const Node>;

    Node();
    virtual ~Node();

    virtual std::string as_string() const = 0;
    void print() { fmt::print("{}\n", as_string()); }

    void set_pos(int l, int c) {
        m_line = l;
        m_col = c;
    }

    template <typename T, typename... Args>
    static auto add(Args &&...args) {
        auto root = std::make_shared<T>(std::forward<Args>(args)...);
        root->set_pos(yylineno, Token::colno);
        s_roots.back() = root;
        return root;
    }

    template <typename T>
    static auto wrap(Node::Ptr node) {
        return std::make_shared<T>(std::vector{node});
    }

    static auto &current_root() {
        assert(! s_roots.empty());
        return s_roots.back();
    }

    virtual bool is_func() const { return false; }
    virtual bool is_class() const { return false; }
    virtual bool is_funcarg_list() const { return false; }
    virtual void set_funcarg_list() { assert(false); }

    virtual bool is_callarg_list() const { return false; }
    virtual bool is_stmt_list() const { return false; }

    virtual bool is_class_stmt_list() const { return false; }
    virtual void set_class_stmt_list() { assert(false); }

    virtual Node::Cptr get_parent() const { return nullptr; }

    struct SymTabEntry {
        std::string name;
        Cptr stmt;

        SymTabEntry() {}
        SymTabEntry(const Ptr s) : stmt(s) {}
        SymTabEntry(const Cptr s) : stmt(s) {}
        SymTabEntry(const std::string &n, const Cptr s = nullptr) : name(n), stmt(s) {}

        operator bool() const { return stmt != nullptr; }
        operator Cptr() const { return stmt; }
        bool first_letter_uppercase() { return name.front() >= 'A' && name.front() <= 'Z'; }
        bool first_letter_lowercase() { return name.front() >= 'a' && name.front() <= 'z'; }
        bool is_builtin() {
            return name == "and" || name == "or" || name == "not" || name == "Boolean"
                    || name == "String" || name == "Integer64";
        }
    };
    /**
     * @brief compute_stmt_type: Computes resulting type of the statement.
     * @param st: The symbol table at hand.
     * @return: Returns nullptr if no errors are found. Otherwise sets the error
     *          string on the statement that caused the error and returns it.
     */
    virtual Ptr compute_stmt_type(SymbolTable &st);

    /**
     * @brief add_to_symtab_forward: If this type needs to be added ahead of time to the symbol
     *        table (eg. func, class), override this function and add it to the symbol table passed
     *        in the arguments
     * @param st: The symbol table at hand.
     * @return: Returns nullptr if no errors are found. Otherwise sets the error
     *          string on the statement that caused the error and returns it.
     */
    virtual Ptr add_to_symtab_forward(SymbolTable &st);

    /**
     * @brief add_to_symtab_ordered: If this type needs to be added in source text order to the
     *        symbol table (eg. let), override this function and add it to the symbol table passed
     *        in the arguments
     * @param st: The symbol table at hand.
     * @return: Returns nullptr if no errors are found. Otherwise sets the error
     *          string on the statement that caused the error and returns it.
     */
    virtual Ptr add_to_symtab_ordered(SymbolTable &st);

    /**
     * @brief get_symbol: Get the statement referred by the current statement in the given symtab.
     *                    Used during type checking phase.
     * @return The statement pointer if found in the given symtab, otherwise nullptr
     */
    virtual SymTabEntry get_symbol(const SymbolTable &) const { return {}; }

    /**
     * @brief get_symbol: Get the statement referred by the current statement in own symtab.
     *                    Used during code generation phase.
     * @return The statement pointer if found in the given symtab, otherwise nullptr
     */
    virtual SymTabEntry get_symbol() const { return {}; }

    /**
     * @brief get_subsymbol: Get the statement referred by the current statement in the symtab of
     *        the given statement. You need to override this function in the statements with
     *        subsymbol access like Module and Class.
     * @return The statement pointer if found in the given symtab, otherwise nullptr
     */
    virtual SymTabEntry get_subsymbol(Ptr) const { return {}; }
    virtual const SymbolTable *get_subsymbol_all() const { return {}; }

    /*
     * Static interface
     */
    static const Ptr &get_root() { return s_roots.back(); }
    static Ptr pop_root();
    static const Ptr &get_root_before();
    static const Ptr &get_first();
    static const Ptr &get_first_before();
    static void reset_root() {
        s_roots.emplace_back();
        s_next_id = 0;
    }
    auto get_line() const { return m_line; }
    auto get_col() const { return m_col; }
    const auto &get_error() const { return m_error; }

    Node::Ptr set_error(const std::string &error) {
        m_error = error;
        return m_error.empty() ? nullptr : shared_from_this();
    }

    const auto &get_stmt_type() const { return m_type; }

    void set_stmt_type(const Cptr &type) {
        assert(type);
        m_type = type;
    }

    void set_cur_symtab(std::shared_ptr<Scope> symtab) {
        assert((! m_cur_symtab) || (m_cur_symtab == symtab));
        m_cur_symtab = symtab;
    }
    auto get_cur_symtab() { return m_cur_symtab; }
    auto get_cur_symtab() const { return m_cur_symtab; }

    virtual Node::Ptr gen_wat(WasmContext &);
    virtual Node::Ptr gen_wat(WasmContext &, const std::string &id) const;

    const auto &get_id() const {
        assert(! m_id.empty());
        return m_id;
    }

    void set_id(const std::string &v) {
        assert(! v.empty());
        m_id = v;
    }

protected:
    std::shared_ptr<Scope> m_cur_symtab;

private:
    static std::vector<Node::Ptr> s_roots;

    Cptr m_type;

    static int64_t s_next_id;
    std::string m_id;
    std::string m_error;
    int m_line = 0;
    int m_col = 0;
};

template <>
struct fmt::formatter<Node> : fmt::formatter<std::string> {
    format_context::iterator format(const Node &stmt, format_context &ctx) const {
        return fmt::formatter<std::string>::format(stmt.as_string(), ctx);
    }
};

template <>
struct fmt::formatter<std::vector<Node::Ptr>> : fmt::formatter<std::string> {
    format_context::iterator
    format(const std::vector<Node::Ptr> &stmts, format_context &ctx) const {
        std::stringstream sstr;
        sstr << "[";

        bool first = true;
        for (const auto &stmt : stmts) {
            if (! first) {
                sstr << ", ";
            }
            first = false;
            sstr << FF("{}", *stmt);
        }

        sstr << "]";

        return fmt::formatter<std::string>::format(sstr.str(), ctx);
    }
};

#endif // KARPUZ_NODE_H
