#ifndef KARPUZ_TEST_WASM_H_
#define KARPUZ_TEST_WASM_H_

#include <memory>
#include <vector>

std::unique_ptr<std::vector<std::string>> run_wasm(std::vector<unsigned char> &code);

#endif
