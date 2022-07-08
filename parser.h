#include "ast.h"
#include "lexer.h"
#include "prog.h"

bool parse (Prog prog, const Token *tokens, size_t tokens_len, ASTAction **actions, size_t *actions_len);
void free_actions (ASTAction *, size_t);
