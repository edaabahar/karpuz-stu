
#pragma once

#include <memory>

#include <fmt/format.h>
#include <fmt/ranges.h>

class Node;
extern "C" int yylex(void);
#define YY_DECL int yylex(void)
#define YYSTYPE std::shared_ptr<Node>
#include "parser.hpp"
