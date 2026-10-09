
#include <cstdio>
#include <memory>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <jsapi.h>

#include <js/ArrayBuffer.h>
#include <js/BigInt.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Initialization.h>
#include <js/SourceText.h>
#include <js/WasmModule.h>
#include <js/experimental/TypedData.h>

// This example illustrates usage of WebAssembly JS API via embedded
// SpiderMonkey. It does no error handling and simply exits if something
// goes wrong.
//
// See 'boilerplate.cpp' for the parts of this example that are reused in many
// simple embedding examples.
//
// To use the WebAssembly JIT you need to create a context and a global object,
// and  do some setup on both of these. You also need to enter a "realm"
// (environment within one global object) before you can execute code.

static JSObject *CreateGlobal(JSContext *cx) {
    JS::RealmOptions options;

    static JSClass BoilerplateGlobalClass = {
            "BoilerplateGlobal", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps};

    return JS_NewGlobalObject(
            cx, &BoilerplateGlobalClass, nullptr, JS::FireOnNewGlobalHook, options);
}

// Helper to read current exception and dump to stderr.
//
// NOTE: This must be called with a JSAutoRealm (or equivalent) on the stack.
static void ReportAndClearException(JSContext *cx) {
    JS::ExceptionStack stack(cx);
    if (! JS::StealPendingExceptionStack(cx, &stack)) {
        fprintf(stderr, "Uncatchable exception thrown, out of memory or something");
        exit(1);
    }

    JS::ErrorReportBuilder report(cx);
    if (! report.init(cx, stack, JS::ErrorReportBuilder::WithSideEffects)) {
        fprintf(stderr, "Couldn't build error report");
        exit(1);
    }

    JS::PrintError(stderr, report, false);
}

static JS::PersistentRootedObject memory_;
std::unique_ptr<std::vector<std::string>> wasm_console_lines;

static bool io_print_b(JSContext *ctx, unsigned argc, JS::Value *vp) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    std::string_view sv;

    auto is_void = args[0].toNumber();
    if (is_void != 0) {
        sv = "void(Boolean)";
    }
    else {
        auto boolval = args[1].toNumber();

        if (boolval == 0) {
            sv = "false";
        }
        else {
            sv = "true";
        }
    }

    fmt::print("{}", sv);

    if (wasm_console_lines->empty()) {
        wasm_console_lines->emplace_back();
    }

    wasm_console_lines->back().insert(wasm_console_lines->back().end(), sv.begin(), sv.end());

    return true;
}

static bool io_print_i(JSContext *ctx, unsigned argc, JS::Value *vp) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    std::string s;
    auto is_void = args[0].toNumber();
    int64_t intval = JS::BigIntToNumber(args[1].toBigInt());
    if (is_void != 0) {
        s = "void(Integer64)";
    }
    else {
        s = std::to_string(intval);
    }

    fmt::print("{}", s);

    if (wasm_console_lines->empty()) {
        wasm_console_lines->emplace_back();
    }

    wasm_console_lines->back().insert(wasm_console_lines->back().end(), s.begin(), s.end());

    return true;
}

static bool io_print_s(JSContext *ctx, unsigned argc, JS::Value *vp) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    std::string_view sv;

    auto is_void = args[0].toNumber();
    if (is_void != 0) {
        sv = "void(String)";
    }
    else {
        size_t offset = args[1].toNumber();
        size_t length = args[2].toNumber();

        JS::RootedObject memobj(ctx, memory_);
        JS::RootedValue abufval(ctx);
        if (! JS_GetProperty(ctx, memobj, "buffer", &abufval)) {
            ReportAndClearException(ctx);
            return false;
        }

        JS::RootedObject abufobj(ctx, &abufval.toObject());

        size_t mem_length = 0;
        bool isSharedMemory = false;
        uint8_t *mem_data = nullptr;
        JS::GetArrayBufferLengthAndData(abufobj, &mem_length, &isSharedMemory, &mem_data);

        auto end = mem_data + mem_length;
        if (mem_data + offset + length > end) {
            return false;
        }
        sv = std::string_view(reinterpret_cast<const char *>(mem_data + offset), length);
    }

    fmt::print("{}", sv);

    if (wasm_console_lines->empty()) {
        wasm_console_lines->emplace_back();
    }

    if (sv == "\n") {
        wasm_console_lines->emplace_back();
    }
    else {
        wasm_console_lines->back().insert(wasm_console_lines->back().end(), sv.begin(), sv.end());
    }

    return true;
}

static std::unique_ptr<std::vector<std::string>>
run_wasm_impl(JSContext *ctx, std::vector<unsigned char> &code) {
    struct Context {
        Context() { wasm_console_lines = std::make_unique<std::vector<std::string>>(); }
        ~Context() { memory_.reset(); }
    } cmgr_;

    auto wasm_data = code.data();
    auto wasm_len = code.size();

    JS::RootedObject global(ctx, CreateGlobal(ctx));
    if (! global) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    JSAutoRealm ar(ctx, global);

    // Get WebAssembly.Module and WebAssembly.Instance constructors.
    JS::RootedValue wasm(ctx);
    JS::RootedValue wasmModule(ctx);
    JS::RootedValue wasmInstance(ctx);
    JS::RootedValue wasmMemory(ctx);
    if (! JS_GetProperty(ctx, global, "WebAssembly", &wasm)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    JS::RootedObject wasmObj(ctx, &wasm.toObject());
    if (! JS_GetProperty(ctx, wasmObj, "Module", &wasmModule)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    if (! JS_GetProperty(ctx, wasmObj, "Instance", &wasmInstance)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    if (! JS_GetProperty(ctx, wasmObj, "Memory", &wasmMemory)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    // Construct Wasm module from bytes.
    JS::RootedObject module_(ctx);
    {
        JSObject *arrayBuffer = JS::NewArrayBufferWithUserOwnedContents(ctx, wasm_len, wasm_data);
        if (! arrayBuffer) {
            ReportAndClearException(ctx);
            return nullptr;
        }
        JS::RootedValueArray<1> args(ctx);
        args[0].setObject(*arrayBuffer);

        if (! Construct(ctx, wasmModule, args, &module_)) {
            ReportAndClearException(ctx);
            return nullptr;
        }
    }

    // Construct Wasm memory
    {
        JS::RootedObject meminitobj(ctx, JS_NewPlainObject(ctx));
        if (! meminitobj) {
            ReportAndClearException(ctx);
            return nullptr;
        }

        JS::RootedValue one(ctx, JS::NumberValue(1));
        if (! JS_SetProperty(ctx, meminitobj, "initial", one)) {
            ReportAndClearException(ctx);
            return nullptr;
        }

        JS::RootedValueArray<1> args(ctx);
        args[0].setObject(*meminitobj);
        memory_.init(ctx);
        if (! Construct(ctx, wasmMemory, args, &memory_)) {
            ReportAndClearException(ctx);
            return nullptr;
        }
    }

    // Construct Wasm module instance with required imports.
    JS::RootedObject instance_(ctx);
    {
        // Build "env" imports object.
        JS::RootedObject envImportObj(ctx, JS_NewPlainObject(ctx));
        if (! envImportObj) {
            ReportAndClearException(ctx);
            return nullptr;
        }
        if (! JS_DefineFunction(ctx, envImportObj, "print_b", io_print_b, 2, 0)) {
            ReportAndClearException(ctx);
            return nullptr;
        }
        if (! JS_DefineFunction(ctx, envImportObj, "print_i", io_print_i, 2, 0)) {
            ReportAndClearException(ctx);
            return nullptr;
        }
        if (! JS_DefineFunction(ctx, envImportObj, "print_s", io_print_s, 3, 0)) {
            ReportAndClearException(ctx);
            return nullptr;
        }

        JS::RootedValue memory_val(ctx, JS::ObjectValue(*memory_));
        if (! JS_SetProperty(ctx, envImportObj, "memory", memory_val)) {
            ReportAndClearException(ctx);
            return nullptr;
        }

        JS::RootedValue envIo(ctx, JS::ObjectValue(*envImportObj));

        // Build imports bag.
        JS::RootedObject imports(ctx, JS_NewPlainObject(ctx));
        if (! imports) {
            ReportAndClearException(ctx);
            return nullptr;
        }
        if (! JS_SetProperty(ctx, imports, "io", envIo)) {
            ReportAndClearException(ctx);
            return nullptr;
        }
        if (! JS_SetProperty(ctx, imports, "memory", memory_val)) {
            ReportAndClearException(ctx);
            return nullptr;
        }

        JS::RootedValueArray<2> args(ctx);
        args[0].setObject(*module_.get()); // module
        args[1].setObject(*imports.get()); // imports

        if (! Construct(ctx, wasmInstance, args, &instance_)) {
            ReportAndClearException(ctx);
            return nullptr;
        }
    }

    // Find `main` method in exports.
    JS::RootedValue exports(ctx);
    if (! JS_GetProperty(ctx, instance_, "exports", &exports)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    JS::RootedObject exportsObj(ctx, &exports.toObject());
    JS::RootedValue main(ctx);
    if (! JS_GetProperty(ctx, exportsObj, "main", &main)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    JS::RootedValue rval(ctx);
    if (! Call(ctx, JS::UndefinedHandleValue, main, JS::HandleValueArray::empty(), &rval)) {
        ReportAndClearException(ctx);
        return nullptr;
    }

    return std::move(wasm_console_lines);
}

// Initialize the JS environment, create a JSContext and run the example
// function in that context. By default the self-hosting environment is
// initialized as it is needed to run any JavaScript). If the 'initSelfHosting'
// argument is false, we will not initialize self-hosting and instead leave
// that to the caller.
std::unique_ptr<std::vector<std::string>> run_wasm(std::vector<unsigned char> &code) {
    /* test_wasmgen.cc 'deki main'e tasindi
    if (! JS_Init()) {
        return false;
    }
    */

    std::unique_ptr<std::vector<std::string>> retval;
    JSContext *cx = JS_NewContext(JS::DefaultHeapMaxBytes);
    if (! cx) {
        return retval;
    }

    if (! JS::InitSelfHostedCode(cx)) {
        JS_DestroyContext(cx);
        return retval;
    }

    if (auto ret = run_wasm_impl(cx, code); (! ret)) {
        JS_DestroyContext(cx);
        return retval;
    }
    else {
        std::swap(ret, retval);
    }

    JS_DestroyContext(cx);

    /* test_wasmgen.cc 'deki main'e tasindi
    JS_ShutDown();
    */

    return retval;
}
