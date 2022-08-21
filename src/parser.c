#include "parser.h"
#include "utf8.h"

#include <stdio.h>
#include <string.h>

#define is_keyword(s, x)  (memcmp(s, x, sizeof(x)-1) == 0 && !is_alpha(*((s) + sizeof(x)-1)))

static inline bool is_space(char c) { return (c == ' ' || c == '\t'); }
static inline bool is_digit(char c) { return ('0' <= c && c <= '9');  }
static inline bool is_alpha(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
static inline bool is_valid(char c);



static Location skip_whitespace(const char* source, Location location) {
    const char* c = source + location.index;
    while (*c == ' ' || *c == '\t' || *c == '\n') {
        if (*c == '\n') {
            location.row += 1;
            location.col  = 0;
        } else {
            location.col += 1;
        }
        location.index += 1;
        c += 1;
    }
    return location;
}


TokenResult parse_string(const char* source, Location location) {
    D_ASSERT(source[location.index] == '"');
    const char* start = source + location.index;
    int cols = 1;

    // @NOTE: Skip '"'
    const char* c = start + 1;
    while (*c != '\0' && *c != '"' && *c != '\n') {
        int i = multi_byte_count(*c);
        c += i;
        cols += 1;
    }


    if (*c != '"') {
        return (TokenResult) { .token=(Token) { .location=location, .type=TOKEN_ERROR }, .error=PARSER_ERROR_UNEXPECTED_EOF, .has_error=true };
    } else {
        // @NOTE: Skip '"'
        c += 1;
        int   count = (int)(c-start);
        Token token = (Token) { .location=location, .count=count, .type=TOKEN_STRING, .cols=cols };
        return (TokenResult) { .token=token, .has_error=false };
    }
}


TokenResult parse_identifier(const char* source, Location location) {
    const char* start = source + location.index;
    // @NOTE: If we've entered, then we've verified that the first
    //  is a valid identifier character.
    const char* c = start + 1;
    int cols = 1;

    while (is_alpha(*c) || is_digit(*c) || *c == '_' || is_continuation_byte(*c)) {
        int i = multi_byte_count(*c);
        c += i;
        cols += 1;
    }

    Token token = (Token) { .location=location, .count=(int)(c-start), .type=TOKEN_IDENTIFIER, .cols=cols  };
    return (TokenResult) { .token=token, .has_error=false };
}


typedef enum {
    PrefixNone,
    PrefixHex,
    PrefixOct,
    PrefixBin
} LiteralPrefix;

typedef enum {
    PostfixNone,
    PostfixF64,
    PostfixI64,
    PostfixU64,
} LiteralPostfix;

TokenResult parse_digit(const char* source, Location location, bool seen_dot) {
    D_ASSERT(seen_dot == false);
    const char* start = source + location.index;
    // @NOTE: If we've entered, then we've verified the first
    //  its a valid digit character.
    const char* c = start + 1;

//    LiteralPrefix prefix = PrefixNone;
//    if (*c == '0') {
//        switch (*(c+1)) {
//            case 'x': prefix = PrefixHex; c += 2; break;
//            case 'o': prefix = PrefixOct; c += 2; break;
//            case 'b': prefix = PrefixBin; c += 2; break;
//        }
//    }

    // @NOTE: Numbers can't start with '_', but if we've entered this function,
    //  then it shouldn't have started on it. We don't restrict where and how
    //  many delimiters are in the number though.
    while (is_digit(*c) || *c == '_')
        c += 1;

    if (*c == '.') {
        seen_dot = true;
        c += 1;
        while (is_digit(*c) || *c == '_')
            c += 1;
    }

//    LiteralPostfix postfix = PostfixNone;
//    switch (*c) {
//        case 'f':
//        case 'i':
//        case 'u':
//
//    }

    int count = (int)(c - start);
    Token token;
    if (seen_dot)
        token = (Token) { .location=location, .count=count, .type=TOKEN_F64, .cols=count };
    else
        token = (Token) { .location=location, .count=count, .type=TOKEN_I64, .cols=count };

    return (TokenResult) { .token=token, .has_error=false };
}


Location skip_line_comment(const char* source, Location location) {
    D_ASSERT(source[location.index] == '/' && source[location.index + 1] == '/');
    const char* start = source + location.index;

    // @NOTE: Skip '//'
    const char* c = start + 2;
    int cols = 0;
    while (*c != '\0' && *c != '\n') {
        int i = multi_byte_count(*c);
        c += i;
        cols += 1;
    }

    int count = (int)(c - start);
    location.col   += cols;
    location.index += count;
    location = skip_whitespace(source, location);
    return location;
}


Location skip_block_comment(const char* source, Location location) {
    int current_it = 0;
    D_ASSERT(source[location.index] == '/' && source[location.index + 1] == '*');

    // @NOTE: Skip '/*'
    location.col   += 2;
    location.index += 2;

    const char* c = source + location.index;
    while (*c != '\0') {
        if (*c == '\n') {
            location.row += 1;
            location.col  = 0;
            location.index += 1;
        } else if (*c == '/') {
            if (*(c-1) == '*') {
                // @NOTE: Skip '/'
                location.col   += 1;
                location.index += 1;
                if (current_it-- == 0)
                    break;
            } else if (*(c+1) == '*') {
                ++current_it;
                location.col   += 2;
                location.index += 2;
                continue;
            }
        } else {
            int count = multi_byte_count(*c);
            location.col   += 1;
            location.index += count;
        }
    }

    location = skip_whitespace(source, location);
    return location;
}



TokenResult token_after(const char* source, Token token) {
#define T(count_, type_) do { result = (TokenResult) { .token = (Token) { .location=(start), .count=(count_), .cols=(count_), .type=(type_) }, .has_error = false, }; goto done; } while(0)
#define S(literal) (sizeof(literal)-1)

    if (token.type == TOKEN_EOF)
        return (TokenResult) {.token = (Token) { .location=token.location, .type=TOKEN_EOF, .count=0, .cols=0 }, .has_error = false };

    TokenResult result;
    Token unknown      = (Token) { .location={0}, .count=0, .cols=0, .type=TOKEN_UNKNOWN};
    bool  seen_dot     = false;
    bool  seen_unknown = false;

    // @NOTE: Step past the token.
    token.location.col   += token.count;
    token.location.index += token.count;
    Location start = skip_whitespace(source, token.location);

    const char* c;
    retry:
    switch (*(c = source + start.index)) {
    case '+':  T(1, TOKEN_PLUS);
    case '-':  T(1, TOKEN_MINUS);
    case '*':  T(1, TOKEN_STAR);
    case '(':  T(1, TOKEN_LEFT_PAREN);
    case ')':  T(1, TOKEN_RIGHT_PAREN);
    case '[':  T(1, TOKEN_LEFT_BRACKET);
    case ']':  T(1, TOKEN_RIGHT_BRACKET);
    case '{':  T(1, TOKEN_LEFT_BRACE);
    case '}':  T(1, TOKEN_RIGHT_BRACE);
    case ';':  T(1, TOKEN_END_STMT);
    case ',':  T(1, TOKEN_COMMA);
    case '!':  if (*(c+1) == '=') T(2, TOKEN_BANG_EQUAL);    else T(1, TOKEN_BANG);
    case '=':  if (*(c+1) == '=') T(2, TOKEN_EQUAL_EQUAL);   else T(1, TOKEN_EQUAL);
    case '<':  if (*(c+1) == '=') T(2, TOKEN_LESS_EQUAL);    else T(1, TOKEN_LESS);
    case '>':  if (*(c+1) == '=') T(2, TOKEN_GREATER_EQUAL); else T(1, TOKEN_GREATER);
    case '\0': T(0, TOKEN_EOF);
    case '"':  result = parse_string(source, start); goto done;
    case '/':
        if (*(c+1) == '/') { start = skip_line_comment(source, start);  goto retry; }
        if (*(c+1) == '*') { start = skip_block_comment(source, start); goto retry; }
        else T(1, TOKEN_SLASH);
    case 'f':
        if (is_keyword(c+1, "alse")) T(S("false"), TOKEN_FALSE);
        if (is_keyword(c+1, "or"))   T(S("for"),   TOKEN_FOR);
        if (is_keyword(c+1, "un"))   T(S("fun"),   TOKEN_FUN);
        break;
    case 'a': if (is_keyword(c+1, "nd"))     T(S("and"),    TOKEN_AND);         goto identifier;
    case 'o': if (is_keyword(c+1, "r"))      T(S("or"),     TOKEN_OR);          goto identifier;
    case 't': if (is_keyword(c+1, "rue"))    T(S("true"),   TOKEN_TRUE);        goto identifier;
    case 'p': if (is_keyword(c+1, "rint"))   T(S("print"),  TOKEN_PRINT_STMT);  goto identifier;
    case 'v': if (is_keyword(c+1, "ar"))     T(S("var"),    TOKEN_VAR);         goto identifier;
    case 'i': if (is_keyword(c+1, "f"))      T(S("if"),     TOKEN_IF);          goto identifier;
    case 'e': if (is_keyword(c+1, "lse"))    T(S("else"),   TOKEN_ELSE);        goto identifier;
    case 'w': if (is_keyword(c+1, "hile"))   T(S("while"),  TOKEN_WHILE);       goto identifier;
    case 'r': if (is_keyword(c+1, "eturn"))  T(S("return"), TOKEN_RETURN);      goto identifier;

    // @TODO: Do we need .<num> syntax? How does that work with prefixes such as 0x, 0o, and 0b?
    // case '.': if (!is_digit(*(c+1))) return T(1, TOKEN_DOT); else seen_dot = true;  // and fall through.
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        result = parse_digit(source, start, seen_dot); goto done;
    }

    default:
        if (is_alpha(*c) || *c == '_') {
            identifier: result = parse_identifier(source, start); goto done;
        }
    }

    // @NOTE: Skip tokens until we find a valid one.
    if (unknown.count == 0) {
        unknown.location = start;
        seen_unknown = true;
    }
    int i = multi_byte_count(*c);
    start.col     += 1;
    unknown.count += i;
    unknown.cols  += 1;
    start.index   += i;
    start = skip_whitespace(source, start);
    goto retry;

    done:
        if (seen_unknown) {
            return (TokenResult) { .token=unknown, .error=PARSER_ERROR_UNKNOWN_TOKEN, .has_error=true };
        } else {
            return result;
        }
}
#undef S
#undef T



Token token_at_offset(const char* source, int offset) {
    Token token = token_make_empty();
    TokenResult result = token_after(source, token);
    while (!result.has_error && result.token.location.index + result.token.count < offset && result.token.type != TOKEN_EOF) {
        result = token_after(source, result.token);
    }

    if (!result.has_error) {
        PANIC("Can't call 'token_at_offset' if you haven't verified it doesn't have errors!");
    } else if (result.token.type == TOKEN_EOF) {
        PANIC("invalid offset '%d'. Could only go to '%d'", offset, result.token.location.index + result.token.count);
    } else {
        return result.token;
    }
}
