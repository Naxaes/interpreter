#include "token.h"

Location token_location(Token token) {
    return token.location;
}

Token token_make_empty() {
    return (Token) { .type=TOKEN_ERROR, .location={ 0, 0, 0 }, .count=0 };
}

Token token_make(TokenType type, Location location, int count) {
    return (Token) { .type=type, .location=location, .count=count };
}
