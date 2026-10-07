#ifndef KARPUZ_TOKEN_H
#define KARPUZ_TOKEN_H

#include "main.h"

class Token {
public:
    using Ptr = std::shared_ptr<Token>;
    using Cptr = std::shared_ptr<const Token>;

    Token(int id) : m_id(id) {}
    virtual ~Token();

    virtual std::string as_string() const = 0;
    void print() { fmt::print("{}\n", as_string()); }

    template <typename T>
    static auto New() {
        return std::make_shared<T>();
    }

    template <typename T, typename... Args>
    static auto New(Args &&...args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    static int colno;

    virtual int get_id() const { return m_id; }

private:
    int m_id;
};

class Rejected : public Token {
public:
    Rejected(const char *text) : Token(YYUNDEF), m_text(text) {}
    std::string as_string() const override { return fmt::format("REJECTED({})", m_text); }

private:
    std::string m_text;
};

namespace token {
inline auto fmt(int v) {
    return static_cast<yytokentype>(v);
}
} // namespace token

#endif // KARPUZ_TOKEN_H
