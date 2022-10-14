#include "ast.h"
#include "prog.h"

bool parse (Prog prog, const Token *tokens, size_t tokens_len, LList *actions);
void free_actions (LList);
