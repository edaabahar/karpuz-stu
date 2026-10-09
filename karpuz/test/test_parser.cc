
#include <gtest/gtest.h>

#include <lexer.hpp>
#include <main.h>

#include <karpuz/Node.h>

extern std::shared_ptr<Token> curtoken;

struct ParserFixture : public testing::Test {
    YY_BUFFER_STATE buffer = nullptr;

    void SetUp() override {
        Node::reset_root();
        // yydebug = 1; // uncomment to your heart's content
    }

    void TearDown() override {
        // Tear down code after each test, even if assertions fail.
        // This will be executed even in the face of assertion failures.
        if (buffer) {
            yy_delete_buffer(buffer);
            buffer = nullptr;
        }

        yydebug = 0;
        Token::colno = 0;
        curtoken.reset();
    }

    void verify_root(const std::string &code, const std::string &ast) {
        buffer = yy_scan_string(code.data());

        /* perform */
        yyparse();

        /* verify */
        ASSERT_TRUE(Node::current_root());
        auto root = Node::current_root();
        ASSERT_EQ(FF("{}", root->as_string()), ast);
    }

    void verify_single(const std::string &code, const std::string &ast) {
        buffer = yy_scan_string(code.data());

        /* perform */
        yyparse();

        /* verify */
        ASSERT_TRUE(Node::current_root());
        auto root = Node::current_root();
        auto root_ast = root->as_string();
        ASSERT_FALSE(root_ast.empty());
        ASSERT_TRUE(root_ast.starts_with("Module(["));
        ASSERT_TRUE(root_ast.ends_with("])"));
        ASSERT_EQ(root_ast.substr(8, root_ast.size() - 10), ast);
    }

    void verify_no_root(const std::string &code) {
        buffer = yy_scan_string(code.data());

        /* perform */
        yyparse();

        /* verify */
        ASSERT_FALSE(Node::current_root());
    }
};

TEST_F(ParserFixture, empty) {
    verify_no_root("");
}

TEST_F(ParserFixture, invalid_char) {
    verify_no_root("=");
}

TEST_F(ParserFixture, plus) {
    verify_single("1+2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, minus) {
    verify_single("1-2;", "Sub(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, mult) {
    verify_single("1*2;", "Mult(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, divf) {
    verify_single("1/2;", "DivF(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, string) {
    verify_single(R"("a";)", "Str(a)");
}

TEST_F(ParserFixture, add_signed) {
    verify_single("1 + -2;", "Add(l=Int(1), r=Signed(OP_MINUS, Int(2)))");
}

TEST_F(ParserFixture, eq) {
    verify_single("a == b;", "OpEq(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, inst) {
    verify_single("a := b;", "Instantiation(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, gt) {
    verify_single("a > b;", "OpGt(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, ge) {
    verify_single("a >= b;", "OpGe(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, lt) {
    verify_single("a < b; ", "OpLt(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, le) {
    verify_single("a <= b;", "OpLe(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, assignment) {
    verify_single("a = 5;", "Assignment(n=Id(a), i=Int(5))");
}

TEST_F(ParserFixture, func) {
    verify_single("fn f() {};", "Fn(n=Id(f), a=[], s=[])");
}

TEST_F(ParserFixture, func_args) {
    verify_single("fn f(a1) {};", "Fn(n=Id(f), a=FnArgs([FArg(n=Id(a1))]), s=[])");
}

TEST_F(ParserFixture, func_args_stmts) {
    verify_single("fn f(a1, a1) { v = 5; };",
            "Fn("
            "n=Id(f), "
            "a=FnArgs([FArg(n=Id(a1)), FArg(n=Id(a1))]), "
            "s=[Assignment(n=Id(v), i=Int(5))]"
            ")");
}

TEST_F(ParserFixture, class) {
    verify_single("class A {};", "Class(n=Id(A), s=[])");
}

TEST_F(ParserFixture, class_method) {
    verify_single("class A { members = a; constructor = f; fn f(b, c) { my.a = b + c; }; };",
            "Class("
                "n=Id(A), "
                "s=["
                    "Members([Id(a)]), "
                    "Constructor(Id(f)), "
                    "Fn("
                        "n=Id(f), "
                        "a=FnArgs("
                            "["
                                "FArg(n=Id(b)), "
                                "FArg(n=Id(c))"
                            "]"
                        "), "
                        "s=["
                            "Assignment("
                                "n=Dot(l=Id(my), r=Id(a))"
                                "i=Add(l=Id(b), r=Id(c))"
                            ")"
                        "]"
                    ")"
                "]"
            ")"
        );
}

TEST_F(ParserFixture, import) {
    verify_single("import a;", "Import(Id(a))");
}

TEST_F(ParserFixture, call) {
    verify_single("a.b.c(d,e);",
            "Call(n=Dot(l=Dot(l=Id(a), r=Id(b)), r=Id(c)), "
            "a=FnArgs([Id(d), Id(e)]))");
}

TEST_F(ParserFixture, if_then_empty) {
    verify_single("if (a) {};", "If(?=Id(a), then=[], else=[])");
}

TEST_F(ParserFixture, if_then_stuff) {
    verify_single("if (a) { x = 0; y = 1; z = 2; };",
            "If(?=Id(a), then=[Assignment(n=Id(x), i=Int(0)), "
            "Assignment(n=Id(y), i=Int(1)), Assignment(n=Id(z), i=Int(2))], else=[])");
}

TEST_F(ParserFixture, if_then_empty_else_empty) {
    verify_single("if (a) { } else { };", "If(?=Id(a), then=[], else=[])");
}

TEST_F(ParserFixture, while_repeat_empty) {
    verify_single("while (a) { };", "While(?=Id(a), repeat=[])");
}

TEST_F(ParserFixture, while_repeat_stuff) {
    verify_single("while (a) { x = 0; };",
            "While(?=Id(a), repeat=[Assignment(n=Id(x), i=Int(0))])");
}

TEST_F(ParserFixture, while_literal) {
    verify_single("while (a > b) {};", "While(?=OpGt(l=Id(a), r=Id(b)), repeat=[])");
}

TEST_F(ParserFixture, labeled_while_break_continue) {
    verify_single("while (true) : outer { continue outer; break outer; };",
            "While(label=Id(outer), ?=Bool(true), "
            "repeat=[Continue(Id(outer)), Break(Id(outer))])");
}

TEST_F(ParserFixture, class_bad_name) {
    verify_no_root("class a.b { };");
}

TEST_F(ParserFixture, func_bad_name) {
    verify_no_root("fn f.g() { };");
}

TEST_F(ParserFixture, func_bad_arg_name) {
    verify_no_root("fn f(foo.bar) { };");
}

TEST_F(ParserFixture, return_literal) {
    verify_single("return 1;", "Return(Int(1))");
}

TEST_F(ParserFixture, bonus) {
    verify_no_root("1---2;");
}
