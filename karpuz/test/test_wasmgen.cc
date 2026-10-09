
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>

// gtest
#include <gtest/gtest.h>
#include <sstream>
#include <string>

// mozjs
#ifdef KARPUZ_HAVE_MOZJS
#include "wasm.h"
#include <js/Initialization.h>
#endif

// wabt
#include <wabt/binary-writer.h>
#include <wabt/error-formatter.h>
#include <wabt/validator.h>
#include <wabt/wast-parser.h>

// karpuz
#include <lexer.hpp>
#include <main.h>

#include <karpuz/Compiler.h>
#include <karpuz/Node.h>

extern int yydebug;

namespace karpuz {

struct WasmGenFixture : public ::testing::Test {
    void SetUp() override {}
    void TearDown() override {}

    /**
     * @brief verify_wat: Verifies the wat output of the given karpuz module
     * @param code: Karpuz source code, as a string.
     * @param ast:  Expected AST for the given code.
     */
    void verify_wat(const std::string &code, const std::string &wat_expected) {
        Compiler compiler;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
            ASSERT_TRUE(Node::get_root_before());
        }

        /* write wat */
        const auto &wat = compiler.get_wasm_ctx().body().str();
        std::string fn = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        auto fn_wat = FF("{}.wat", fn);
        auto fn_wasm = FF("{}.wasm", fn);

        {
            std::ofstream f;
            f.open(fn_wat);
            if (f.is_open()) {
                f.write(wat.data(), wat.size());
            }
        }

        /* generate wasm from wat */
        std::vector<uint8_t> wasm;
        {
            wabt::Features features;
            wabt::Errors errors;

            // wat: run lexer
            std::unique_ptr<wabt::WastLexer> lexer = wabt::WastLexer::CreateBufferLexer(
                    std::string_view(fn_wat), wat.data(), wat.size(), &errors);

            // wat: run parser
            std::unique_ptr<wabt::Module> module;
            wabt::WastParseOptions parse_wast_options(features);
            if (auto result = ParseWatModule(lexer.get(), &module, &errors, &parse_wast_options);
                    Failed(result)) {
                auto line_finder = lexer->MakeLineFinder();
                FormatErrorsToFile(errors, wabt::Location::Type::Text, line_finder.get());
                ASSERT_TRUE(Succeeded(result));
            }

            // wasm: validate module code
            wabt::ValidateOptions options(features);
            if (auto result = ValidateModule(module.get(), &errors, options); Failed(result)) {
                auto line_finder = lexer->MakeLineFinder();
                FormatErrorsToFile(errors, wabt::Location::Type::Text, line_finder.get());
                ASSERT_TRUE(Succeeded(result));
            }

            // wasm: write to memory buffer
            wabt::MemoryStream stream;
            wabt::WriteBinaryOptions write_binary_options;
            if (auto result = WriteBinaryModule(&stream, module.get(), write_binary_options);
                    Failed(result)) {
                auto line_finder = lexer->MakeLineFinder();
                FormatErrorsToFile(errors, wabt::Location::Type::Text, line_finder.get());
                ASSERT_TRUE(Succeeded(result));
            }

            // wasm: write to file
            {
                std::ofstream f;
                f.open(fn_wasm);
                if (f.is_open()) {
                    auto data = stream.output_buffer().data.data();
                    auto size = stream.output_buffer().data.size();
                    auto written = f.write(reinterpret_cast<const char *>(data), size).tellp();
                    ASSERT_EQ(size, written);
                }
            }

            std::swap(wasm, stream.output_buffer().data);
        }

        ASSERT_EQ(wat, wat_expected);
    }

#ifdef KARPUZ_HAVE_MOZJS
    /**
     * @brief verify_output: Verifies the wat output of the given karpuz module
     * @param code: Karpuz source code, as a string.
     * @param ast:  Expected AST for the given code.
     */
    void verify_output(const std::string &code, const std::vector<std::string> &lines_expected) {
        Compiler compiler;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
            ASSERT_TRUE(Node::get_root_before());
        }

        /* write wat */
        const auto &wat = compiler.get_wasm_ctx().body().str();
        std::string fn = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        auto fn_wat = FF("{}.wat", fn);
        auto fn_wasm = FF("{}.wasm", fn);

        {
            std::ofstream f;
            f.open(fn_wat);
            if (f.is_open()) {
                f.write(wat.data(), wat.size());
            }
        }

        /* generate wasm from wat */
        std::vector<uint8_t> wasm;
        {
            wabt::Features features;
            wabt::Errors errors;

            // wat: run lexer
            std::unique_ptr<wabt::WastLexer> lexer = wabt::WastLexer::CreateBufferLexer(
                    std::string_view(fn_wat), wat.data(), wat.size(), &errors);

            // wat: run parser
            std::unique_ptr<wabt::Module> module;
            wabt::WastParseOptions parse_wast_options(features);
            if (auto result = ParseWatModule(lexer.get(), &module, &errors, &parse_wast_options);
                    Failed(result)) {
                auto line_finder = lexer->MakeLineFinder();
                FormatErrorsToFile(errors, wabt::Location::Type::Text, line_finder.get());
                ASSERT_TRUE(Succeeded(result));
            }

            // wasm: validate module code
            wabt::ValidateOptions options(features);
            if (auto result = ValidateModule(module.get(), &errors, options); Failed(result)) {
                auto line_finder = lexer->MakeLineFinder();
                FormatErrorsToFile(errors, wabt::Location::Type::Text, line_finder.get());
                ASSERT_TRUE(Succeeded(result));
            }

            // wasm: write to memory buffer
            wabt::MemoryStream stream;
            wabt::WriteBinaryOptions write_binary_options;
            if (auto result = WriteBinaryModule(&stream, module.get(), write_binary_options);
                    Failed(result)) {
                auto line_finder = lexer->MakeLineFinder();
                FormatErrorsToFile(errors, wabt::Location::Type::Text, line_finder.get());
                ASSERT_TRUE(Succeeded(result));
            }

            // wasm: write to file
            {
                std::ofstream f;
                f.open(fn_wasm);
                if (f.is_open()) {
                    auto data = stream.output_buffer().data.data();
                    auto size = stream.output_buffer().data.size();
                    auto written = f.write(reinterpret_cast<const char *>(data), size).tellp();
                    ASSERT_EQ(size, written);
                }
            }

            std::swap(wasm, stream.output_buffer().data);
        }

        /* run wasm using mozjs*/
        auto lines = run_wasm(wasm);
        ASSERT_TRUE(lines);
        ASSERT_EQ(*lines, lines_expected);
    }
#else
    void verify_output(const std::string &code, const std::vector<std::string> &lines_expected) {
        ASSERT_TRUE(false);
    }
#endif
};
// tests

} // namespace karpuz

int main(int argc, char **argv) {
#ifdef KARPUZ_HAVE_MOZJS
    if (! JS_Init()) {
        return 1;
    }
#endif

    testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();

#ifdef KARPUZ_HAVE_MOZJS
    JS_ShutDown();
#endif

    return ret;
}
