
%option noyywrap
%option yylineno

%{
// https://stackoverflow.com/questions/9611682/flexlexer-support-for-unicode/9617585#9617585
#include "main.h"
#include <karpuz/token/Literal.h>
#include <karpuz/token/Operator.h>
static auto &colno = Token::colno;
using namespace token;
extern std::shared_ptr<Token> curtoken;
%}


%%

.        ;
