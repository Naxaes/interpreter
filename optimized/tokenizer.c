#include "tokenizer.h"
#include "utf8.h"
#include "slice.h"
#include <stdlib.h>
#include "nax_logging/nax_logging.h"


typedef enum {
    TOKEN_OPT_ERROR_UNEXPECTED_EOF = TOKEN_COUNT+1,
    TOKEN_OPT_ERROR_BEGIN = TOKEN_OPT_ERROR_UNEXPECTED_EOF,
    TOKEN_OPT_ERROR_UNKNOWN_TOKEN,
    TOKEN_OPT_ERROR_END = TOKEN_OPT_ERROR_UNKNOWN_TOKEN,
    TOKEN_OPT_COUNT,
} TokenTypeOpt;


typedef struct {
    TokenTypeOpt type : 8;
    u64 offset : 24;
    u64 row    : 20;
    u64 column : 12;
} TokenOpt;


typedef struct {
    TokenOpt token;
    Location next;
} TokenResult;



Location token_location(Token token) {
    Location location;
    memcpy(&location, &token, sizeof(Location));
    return location;
}

static const char* ERROR_MESSAGE[] = {
        [TOKEN_OPT_ERROR_UNKNOWN_TOKEN]   = "Unknown token '%.*s'",
        [TOKEN_OPT_ERROR_UNEXPECTED_EOF]  = "Unexpected EOF",
};


static inline bool token_is_error(int type) {
    if (TOKEN_OPT_ERROR_BEGIN <= type && type <= TOKEN_OPT_ERROR_END)
        return true;
    else
        return false;
}



Location token_opt_location(TokenOpt token) {
    Location location;
    memcpy(&location, &token, sizeof(Location));
    return location;
}


typedef struct {
    TokenOpt    token;
    const char* path;
    const char* source;
} TokenizerError;


typedef struct {
    TokenOpt token;
    Location ends_at;
} TokenView;


static inline TokenOpt make_token(TokenType type, Location location) {
    nax_assert(type < 255 && !token_is_error(type));
    TokenOpt token;
    memcpy(&token, &location, sizeof(token));
    token.type = (TokenTypeOpt) type;
    return token;
}

static inline TokenOpt make_error(Location location, TokenTypeOpt error) {
    nax_assert(token_is_error(error));
    TokenOpt token;
    memcpy(&token, &location, sizeof(token));
    token.type = error;
    return token;
}


static inline TokenResult make_result(TokenOpt token, Location next) {
    return (TokenResult) { .token=token, .next=next };
}

static inline TokenView make_view(TokenOpt token, Location end) {
    return (TokenView) { .token=token, .ends_at=end };
}

static inline Slice slice_from_view(const char* source, TokenView view) {
    Location location = token_opt_location(view.token);
    int count = (int) (view.ends_at.offset - location.offset);
    return (Slice) { .source=source+location.offset, .count=count };
}


#define is_keyword(s, x)  (memcmp(s, x, sizeof(x)-1) == 0 && !is_alpha(*((s) + sizeof(x)-1)))




static Location skip_whitespace(const char* source, Location location) {
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    const char* c = source + offset;
    while (*c == ' ' || *c == '\t' || *c == '\n') {
        if (*c == '\n') {
            row   += 1;
            column = 1;
        } else {
            column += 1;
        }
        offset += 1;
        c += 1;
    }
    return make_location(offset, row, column);
}


static Location skip_line_comment(const char* source, Location location) {
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    nax_assert(source[offset] == '/' && source[offset + 1] == '/');
    const char* start = source + offset;

    // @NOTE: Skip '//'
    const char* c = start + 2;
    column += 2;
    while (*c != '\0' && *c != '\n') {
        int i = multi_byte_count(*c);
        c += i;
        column += 1;
    }



    u16 count = (u16)(c - start);
    return make_location(offset+count, row, column);
}


static Location skip_block_comment(const char* source, Location location) {
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    nax_assert(source[offset] == '/' && source[offset + 1] == '*');
    int current_it = 0;

    // @NOTE: Skip '/*'
    column += 2;
    offset += 2;

    const char* c;
    while (*(c = source + offset) != '\0') {
        if (*c == '\n') {
            row += 1;
            column  = 1;
            offset += 1;
        } else if (*c == '/') {
            if (*(c-1) == '*') {
                // @NOTE: Skip '/'
                column += 1;
                offset += 1;
                if (current_it-- == 0)
                    break;
            } else if (*(c+1) == '*') {
                ++current_it;
                column += 2;
                offset += 2;
                continue;
            }
        } else {
            int count = multi_byte_count(*c);
            column += 1;
            offset += count;
        }
    }

    return make_location(offset, row, column);
}



static Location skip_to_next(const char* source, Location location) {
    const char* c = source + location.offset;
    while (true) {
        if (c[0] == '/') {
            if (c[1] == '/') {
                location = skip_line_comment(source, location);
                c = source + location.offset;
            }
            else if (c[1] == '*') {
                location = skip_block_comment(source, location);
                c = source + location.offset;
            } else {
                break;
            }
        } else if (is_whitespace(c[0])) {
            location = skip_whitespace(source, location);
            c = source + location.offset;
        } else {
            break;
        }
    }
    return location;
}


static TokenView parse_string(const char* source, Location location) {
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    nax_assert(source[location.offset] == '"');
    const char* start = source + offset;

    // @NOTE: Skip '"'
    const char* c = start + 1;
    column += 1;
    while (*c != '\0' && *c != '"' && *c != '\n') {
        int i = multi_byte_count(*c);
        c += i;
        column += 1;
    }


    if (*c != '"') {
        TokenOpt token = make_error(location, TOKEN_OPT_ERROR_UNEXPECTED_EOF);
        u64 count = (u64)(c - start);
        location = make_location(offset+count, row, column);
        return make_view(token, location);
    } else {
        // @NOTE: Skip '"'
        TokenOpt token = make_token(TOKEN_STRING, location);
        c += 1;
        column += 1;
        u64 count = (u64)(c - start);
        location = make_location(offset+count, row, column);
        return make_view(token, location);
    }
}


static TokenView parse_identifier(const char* source, Location location) {
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    const char* start = source + offset;
    // @NOTE: If we've entered, then we've verified that the first
    //  is a valid identifier character.
    const char* c = start + 1;
    column += 1;

    while (is_alpha(*c) || is_digit(*c) || *c == '_' || is_continuation_byte(*c)) {
        int i = multi_byte_count(*c);
        c += i;
        column += 1;
    }

    offset += (u64) (c - start);

    TokenOpt token = make_token(TOKEN_IDENTIFIER, location);
    location = make_location(offset, row, column);
    return make_view(token, location);
}


static TokenView parse_number(const char* source, Location location) {
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    const char* start = source + offset;
    // @NOTE: If we've entered, then we've verified the first
    //  its a valid digit character.
    const char* c = start + 1;
    bool seen_dot = false;

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

    TokenOpt token;
    if (seen_dot)
        token = make_token(TOKEN_F64, location);
    else
        token = make_token(TOKEN_I64, location);

    u16 count = (u16)(c - start);
    location = make_location(offset+count, row, column+count);
    return make_view(token, location);
}


static TokenView parse_unknown(const char* source, Location location) {
    static const Slice VALID_SYMBOLS = SLICE(" \0\t\n+-*()[]{};,!=<>\"");
    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    const char* c = source + offset;

    do {
        offset += multi_byte_count(*c);
        column += 1;

        c = source + offset;
        if (slice_contains(VALID_SYMBOLS, *c) || is_alpha(*c) || is_digit(*c)) {
            TokenOpt token = make_error(location, TOKEN_OPT_ERROR_UNKNOWN_TOKEN);
            location = make_location(offset, row, column);
            return make_view(token, location);
        }
    } while (true);
}


static_assert(TOKEN_OPT_ERROR_END-TOKEN_OPT_ERROR_BEGIN == 1, "assuming_error");
TokenView token_opt_view(const char* source, TokenOpt token) {
    Location location = token_opt_location(token);
    switch (token.type) {
        case TOKEN_OPT_ERROR_UNEXPECTED_EOF:
            return make_view(token, location);
        case TOKEN_OPT_ERROR_UNKNOWN_TOKEN: {
            TokenView view = parse_unknown(source, location);
            return view;
        }
        default:
            nax_panic("Should not happen!");
    }

    nax_panic("Should not happen!");
}




TokenResult token_at(const char* source, Location location) {
#define T(count_, type_) do {                   \
    token = make_token(type_, location);        \
    location = make_location(offset+(count_), row, column+(count_)); \
    location = skip_to_next(source, location);  \
    return make_result(token, location);        \
} while(0)
#define S(literal) (sizeof(literal)-1)



    TokenOpt token;
    const char* c;

    retry:;

    u64 offset = location.offset;
    u64 row    = location.row;
    u64 column = location.column;

    switch (*(c = source + location.offset)) {
        case '\0': T(0, TOKEN_EOF);
        case '\n':
        case '\t':
        case ' ':  location = skip_to_next(source, location); goto retry;
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
        case '"':  {
            TokenView view = parse_string(source, location);
            location = view.ends_at;
            token = view.token;
            goto done;
        }
        case '/':
            if (*(c+1) == '/' || *(c+1) == '*') { location = skip_to_next(source, location); goto retry; }
            else T(1, TOKEN_SLASH);
        case 'a': if (is_keyword(c+1, "nd"))     T(S("and"),    TOKEN_AND);        else goto identifier;
        case 'o': if (is_keyword(c+1, "r"))      T(S("or"),     TOKEN_OR);         else goto identifier;
        case 't': if (is_keyword(c+1, "rue"))    T(S("true"),   TOKEN_TRUE);       else goto identifier;
        case 'v': if (is_keyword(c+1, "ar"))     T(S("var"),    TOKEN_VAR);        else goto identifier;
        case 'i': if (is_keyword(c+1, "f"))      T(S("if"),     TOKEN_IF);         else goto identifier;
        case 'e': if (is_keyword(c+1, "lse"))    T(S("else"),   TOKEN_ELSE);       else goto identifier;
        case 'w': if (is_keyword(c+1, "hile"))   T(S("while"),  TOKEN_WHILE);      else goto identifier;
        case 'r': if (is_keyword(c+1, "eturn"))  T(S("return"), TOKEN_RETURN);     else goto identifier;
        case 'f':
            if (is_keyword(c+1, "alse")) T(S("false"), TOKEN_FALSE);
            if (is_keyword(c+1, "or"))   T(S("for"),   TOKEN_FOR);
            if (is_keyword(c+1, "un"))   T(S("fun"),   TOKEN_FUN);
            break;

            // @TODO: Do we need .<num> syntax? How does that work with prefixes such as 0x, 0o, and 0b?
            // case '.': if (!is_digit(*(c+1))) return T(1, TOKEN_DOT);  // and fall through.
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
            TokenView view = parse_number(source, location);
            location = view.ends_at;
            token = view.token;
            goto done;
        }

        default:
            if (is_alpha(*c) || *c == '_') {
                identifier:;
                TokenView view = parse_identifier(source, location);
                location = view.ends_at;
                token = view.token;
                goto done;
            }
    }

    // Error!
    TokenView view = parse_unknown(source, location);
    location = view.ends_at;
    token = view.token;
    // Fallthrough

    done:
    location = skip_to_next(source, location);
    return make_result(token, location);

#undef S
#undef T
}


Slice token_view(const char* source, Token token) {
    Location location = token_location(token);
    switch (token.type) {
        case TOKEN_LEFT_PAREN:    return SLICE("(");
        case TOKEN_RIGHT_PAREN:   return SLICE(")");
        case TOKEN_LEFT_BRACKET:  return SLICE("[");
        case TOKEN_RIGHT_BRACKET: return SLICE("]");
        case TOKEN_LEFT_BRACE:    return SLICE("{");
        case TOKEN_RIGHT_BRACE:   return SLICE("}");

        case TOKEN_MINUS:   return SLICE("-");
        case TOKEN_PLUS:    return SLICE("+");
        case TOKEN_SLASH:   return SLICE("/");
        case TOKEN_STAR:    return SLICE("*");
        case TOKEN_PERCENT: return SLICE("%");
        case TOKEN_U64:
        case TOKEN_I64:
        case TOKEN_F64: {
            TokenView view = parse_number(source, location);
            return slice_from_view(source, view);
        }

        case TOKEN_FALSE: return SLICE("false");
        case TOKEN_TRUE:  return SLICE("true");

        case TOKEN_EQUAL: return SLICE("=");
        case TOKEN_BANG:  return SLICE("!");

        case TOKEN_BANG_EQUAL:    return SLICE("!=");
        case TOKEN_EQUAL_EQUAL:   return SLICE("==");
        case TOKEN_GREATER:       return SLICE(">");
        case TOKEN_GREATER_EQUAL: return SLICE(">=");
        case TOKEN_LESS:          return SLICE("<");
        case TOKEN_LESS_EQUAL:    return SLICE("<=");

        case TOKEN_AND: return SLICE("and");
        case TOKEN_OR:  return SLICE("or");

        case TOKEN_STRING: {
            TokenView view = parse_string(source, location);
            return slice_from_view(source, view);
        }
        case TOKEN_IDENTIFIER: {
            TokenView view = parse_identifier(source, location);
            return slice_from_view(source, view);
        }

        case TOKEN_VAR:      return SLICE("var");
        case TOKEN_FUN:      return SLICE("fun");

        case TOKEN_IF:       return SLICE("if");
        case TOKEN_ELSE:     return SLICE("else");

        case TOKEN_WHILE:    return SLICE("while");
        case TOKEN_FOR:      return SLICE("for");
        case TOKEN_RETURN:   return SLICE("return");

        case TOKEN_COMMA:    return SLICE(",");
        case TOKEN_DOT:      return SLICE(".");

        case TOKEN_END_STMT: return SLICE(";");
        case TOKEN_EOF:      return SLICE("\0");

        case TOKEN_COUNT:
            nax_panic("Should not happen!");
    }

    nax_panic("Should not happen!");
}





static inline TokenizerError make_tokenizer_error(const char* source, const char* path, TokenOpt token) {
    nax_assert(token_is_error(token.type));
    return (TokenizerError) {
        .token=token,
        .source=source,
        .path=path,
    };
}


static void print_error(TokenizerError error) {
    Location  start = token_opt_location(error.token);
    TokenView view  = token_opt_view(error.source, error.token);
    Slice repr = slice_from_view(error.source, view);

    int columns = view.ends_at.column - start.column;

    if (error.token.type == TOKEN_OPT_ERROR_UNEXPECTED_EOF)
        repr = SLICE("EOF");

    Slice line_before = previous_line(error.source, start.offset);
    Slice line_on     = current_line(error.source,  start.offset);
    Slice line_after  = next_line(error.source,     start.offset);

    char arrow_buffer[1024] = { 0 };
    int n = (start.column-1 < 1022) ? start.column-1 : 1022;
    for (int j = 0; j < n; ++j) {
        arrow_buffer[j] = '-';  // (i % 5 == 4) ? '*' : '-';
    }
    int m = (n+columns < 1023) ? n+columns : 1023;
    for (int j = n; j < m; ++j) {
        arrow_buffer[j] = '^';  // (i % 5 == 4) ? '*' : '-';
    }
    arrow_buffer[m] = '\0';

    fprintf(
            stderr, "[%s] Error at %s:%d:%d:\n    ",
            "Tokenizer",
            error.path,
            start.row, start.column
    );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wformat-security"
    if (!slice_is_empty(repr)) {
        fprintf(stderr, ERROR_MESSAGE[error.token.type], repr.count, repr.source);
    } else {
        fprintf(stderr, ERROR_MESSAGE[error.token.type]);
    }
#pragma clang diagnostic pop

    fprintf(stderr, ".\n");
    if (line_before.count > 0) {
        fprintf(stderr, "    %-4d| %.*s\n", start.row-1, line_before.count, line_before.source);
    }
    fprintf(stderr,
            "    %-4d| %.*s\n"
            "        |-%s\n",
            start.row, line_on.count, line_on.source,
            arrow_buffer
    );

    if (line_after.count > 0) {
        fprintf(stderr, "    %-4d| %.*s\n", start.row+1, line_after.count, line_after.source);
    }
}


Array_Token tokenize(const char* source, const char* path, StackAllocator* allocator) {
    Token* all_tokens = stack_top(*allocator, Token);

    int count = 0;
    Location location = make_location(0, 1, 1);

    TokenizerError* errors = 0;
    int error_count = 0;

    TokenResult result = token_at(source, location);
    while (result.token.type != TOKEN_EOF) {
        if (token_is_error(result.token.type)) {
            if (error_count == 0) {
                errors = ALLOCATE_ARRAY(TokenizerError, 32);
            }
            if (error_count != 32) {
                errors[error_count++] = make_tokenizer_error(source, path, result.token);
            }
        } else {
            stack_push(allocator, Token, result.token);
        }
        result = token_at(source, result.next);
        ++count;
    }
    stack_push(allocator, Token, result.token);
    ++count;

    if (error_count != 0) {
        for (int i = 0; i < error_count; ++i) {
            print_error(errors[i]);
        }
        exit(EXIT_FAILURE);
    }

    return make_array_Token(all_tokens, count);
}
