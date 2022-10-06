#include "ast.h"
#include "prog.h"
#include "list.h"

GENLIST_TYPE(ASTAction, Action, action)

bool parse (Prog prog, const Token *tokens, size_t tokens_len, LLAction *actions);
void free_actions (LLAction);
