#include "token.h"

Token token_make_empty() {
    return (Token) { .type=TOKEN_ERROR, .location={ 1, 0, 0 }, .count=0, .cols=0 };
}
