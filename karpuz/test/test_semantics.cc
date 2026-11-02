
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

TEST_F(CompilerFixture, class_near) {
    verify_ok("class P {}; class C:P {};");
}

TEST_F(CompilerFixture, class_near_missing) {
    verify_error("class C:P {};", "Type 'P' is not found");
}

TEST_F(CompilerFixture, import_missing) {
    verify_error("fn f() { io.foo.bar();\n };", "Identifier 'io' is not found");
}

TEST_F(CompilerFixture, func_return_operand_missing) {
    verify_error("fn f() { a = 5; return a + b; };", "Identifier 'b' is not found");
}

TEST_F(CompilerFixture, func_untyped_arg) {
    verify_ok("fn f(a) { };");
}

TEST_F(CompilerFixture, func_adjacent) {
    verify_ok("fn f(a) { };"
              "fn g(a) { };");
}

TEST_F(CompilerFixture, var_missing) {
    verify_error("fn f() { return a; };", "Identifier 'a' is not found");
}

TEST_F(CompilerFixture, variable_reassignment_different_type) {
    verify_ok(R"(fn f() { a = 5; a = "aaa"; };)");
}

TEST_F(CompilerFixture, var_scoped_reassignment) {
    verify_ok("a = 5; fn f() { a = 5; };");
}

TEST_F(CompilerFixture, var_scoped) {
    verify_ok("fn f() { a = 5; }; a = 5;");
}

TEST_F(CompilerFixture, local_assignment_shadows_far_variable) {
    verify_ok(R"(a = 5; fn f() { a = "local"; };)");
}

TEST_F(CompilerFixture, far_assignment_updates_far_variable) {
    verify_ok("a = 5; fn f() { far a; a = 10; };");
}

TEST_F(CompilerFixture, far_cannot_follow_local_assignment) {
    verify_error(
            "fn f() { a = 5; far a; };", "Far declaration 'a' conflicts with a local declaration");
}

TEST_F(CompilerFixture, class_members_constructor_and_my) {
    verify_ok(R"(class C {
        members = a, b, c;
        constructor = lala;
        fn lala() { my.a = 5; };
    };)");
}

TEST_F(CompilerFixture, class_attributes_are_private) {
    verify_error(R"(class C { members = value; }; fn read() { return C.value; };)",
            "Class attribute 'value' is private");
}

TEST_F(CompilerFixture, class_member_must_be_declared) {
    verify_error(R"(class C { members = value; fn f() { my.other = 5; }; };)",
            "Member 'other' is not declared");
}

TEST_F(CompilerFixture, members_must_be_declared_in_a_class) {
    verify_error("members = value;", "Members declaration is only valid inside a class");
}

TEST_F(CompilerFixture, constructor_must_name_a_method) {
    verify_error("class C { constructor = init; };", "Constructor 'init' is not a method");
}

TEST_F(CompilerFixture, my_must_be_used_inside_a_class) {
    verify_error("fn f() { my.value = 5; };", "'my' is only valid inside a class");
}

TEST_F(CompilerFixture, class_scope_var_func_conflict) {
    verify_error("class A { members = a; fn a() {}; };", "Identifier 'a' is already in symtab");
}

TEST_F(CompilerFixture, func_name_conflict) {
    verify_error("class A { }; fn A() {};", "Identifier 'A' is already in symtab");
}

TEST_F(CompilerFixture, func_has_Boolean) {
    verify_ok("fn m() { return true; };");
}

TEST_F(CompilerFixture, class_method_named_like_class) {
    verify_ok("class A { fn A() {}; };");
}

TEST_F(CompilerFixture, class_method_no_conflict) {
    verify_ok("class A { fn A() {}; };");
}

TEST_F(CompilerFixture, class_fn_conflict) {
    verify_error("class A {}; fn A() {};", "Identifier 'A' is already in symtab");
}

TEST_F(CompilerFixture, class_method_dup) {
    verify_error("class A { fn A() {}; fn A() {}; };", "Identifier 'A' is already in symtab");
}

TEST_F(CompilerFixture, func_has_String) {
    verify_ok("fn m() { return \"s\"; };");
}

TEST_F(CompilerFixture, func_no_builtin_assignment_and) {
    verify_error("fn m() { and = or; };", "Overriding builtin 'and' is not allowed");
}

TEST_F(CompilerFixture, func_hello_world) {
    verify_ok(R"(import io; fn main() { io.print("Hello world!\n"); };)");
}

TEST_F(CompilerFixture, func_return) {
    verify_ok(R"(fn f() { return 0; };)");
}

TEST_F(CompilerFixture, return_misplaced_module) {
    verify_error(R"(return a;)", "Misplaced return statement");
}

TEST_F(CompilerFixture, while_simple) {
    verify_ok(R"(fn f() {while(true) {};};)");
}

TEST_F(CompilerFixture, labeled_break_and_continue) {
    verify_ok(R"(fn f() { while(true) : outer { break outer; continue outer; }; };)");
}

TEST_F(CompilerFixture, labeled_break_target_missing) {
    verify_error(
            R"(fn f() { break outer; };)", "Break target 'outer' is not an enclosing labeled loop");
}

TEST_F(CompilerFixture, labeled_continue_target_missing) {
    verify_error(R"(fn f() { while(true) { continue outer; }; };)",
            "Continue target 'outer' is not an enclosing labeled loop");
}

TEST_F(CompilerFixture, lt_validates_left_operand) {
    verify_error(R"(fn f() { if (missing < 1) {}; };)", "Identifier 'missing' is not found");
}

TEST_F(CompilerFixture, if_complex_lt) {
    verify_ok(R"(class A{ members = i; fn f() { if(my.i<5) {} else {}; }; };)");
}

TEST_F(CompilerFixture, if_simple) {
    verify_ok(R"(fn f(){if(true) {};};)");
}

TEST_F(CompilerFixture, else_if) {
    verify_ok(R"(fn f(){if(false){} else if(true){} else {};};)");
}

TEST_F(CompilerFixture, func_use_before_definition) {
    verify_ok(R"(fn say_hello() {
            h = get_hello();
        };
        fn get_hello() {
            return "Hello, World!\n";
        };
        )");
}

TEST_F(CompilerFixture, func_missing) {
    verify_error("fn say_hello() { h = get_hello(); };", "Identifier 'get_hello' is not found");
}

TEST_F(CompilerFixture, call_target_must_be_a_function) {
    verify_error("fn f() { value = 1; value(); };", "Identifier 'value' is not a function");
}

TEST_F(CompilerFixture, import_subsymbol_missing) {
    verify_error("import io; fn f() { io.missing(); };", "Identifier 'io.missing' is not found");
}

TEST_F(CompilerFixture, func_call_untyped_parameter) {
    verify_ok("fn g(s) { };fn f() { g(42); };");
}

} // namespace karpuz
