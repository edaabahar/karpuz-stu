
#include <regex>

#include <gtest/gtest.h>

#include <lexer.hpp>
#include <main.h>

#include <karpuz/Compiler.h>
#include <karpuz/Node.h>

extern int yydebug;

namespace karpuz {

struct CompilerFixture : public ::testing::Test {
    void SetUp() override {}
    void TearDown() override {}

    /**
     * @brief verify_first: Verifies the AST of the given module.
     * @param code: Karpuz source code, as a string.
     * @param ast:  Expected AST for the given code.
     */
    void verify_root(const std::string &code, const std::string &ast) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
        }

        ASSERT_TRUE(Node::get_root_before());
        auto root = Node::get_root_before();
        ASSERT_EQ(FF("{}", *root), ast);
    }

    void verify_ok(const std::string &code) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
        }

        ASSERT_TRUE(Node::get_root_before());
    }

    /**
     * @brief verify_first: Verifies the AST of the first statement in the given
     * module.
     * @param code: Karpuz source code, as a string.
     * @param ast:  Expected AST for the given code.
     */
    void verify_first(const std::string &code, const std::string &ast) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
        }

        ASSERT_TRUE(Node::get_root_before());
        ASSERT_TRUE(Node::get_first_before());
        auto first = Node::get_first_before();
        ASSERT_EQ(FF("{}", *first), ast);
    }

    void verify_error(const std::string &code) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (Node::get_root_before()) {
            fmt::print("ERR?: {}\n", *Node::get_root_before());
        }

        ASSERT_FALSE(Node::get_root_before());
    }

    void verify_error(const std::string &code, const std::string &err) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (Node::get_root_before()) {
            fmt::print("ERR?: {}\n", *Node::get_root_before());
        }

        ASSERT_FALSE(Node::get_root_before());
        ASSERT_EQ(std::regex_replace(compiler.get_error(),
                          std::regex("^Error at [0-9]+:[0-9]+: (.*)\n$"), "$1"),
                err);
    }
};

// tests

} // namespace karpuz
