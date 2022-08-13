#include "parser.h"
#include <stdio.h>
#include <string.h>

#define is_keyword(s, x)  (memcmp(s, x, sizeof(x)-1) == 0 && !is_alpha(*((s) + sizeof(x)-1)))

static inline bool is_space(char c);
static inline bool is_digit(char c);
static inline bool is_alpha(char c);
static inline bool is_valid(char c);
static int  skip_space(Parser* self);
static void skip_whitespace(Parser* self);
static void parser_next(Parser* self, int count, TokenType type);
static void parser_string(Parser* self);
static void parser_comment(Parser* self);
static void parser_digit(Parser* self);
static void parser_end(Parser* self);


void parser_unknown_token(Parser* self);

Parser parser_make(const char* path, const char* source) {
    Parser parser;
    parser.path = path;
    parser.source = source;
    parser.location = (Location) { .row=1, .col=0, .index=0 };
    parser.previous = token_make_empty();
    parser.current  = token_make_empty();
    parser.in_panic_mode = false;
    memset(parser.errors, 0, sizeof(parser.errors));
    parser.error_count = 0;
    return parser;
}


bool parser_had_error(const Parser* self) {
    return self->error_count > 0;
}

void parser_flush_errors(Parser* self) {
    for (int i = 0; i < self->error_count; ++i) {
        Error error = self->errors[i];
        print_error(error);
    }
    self->error_count = 0;
}


void parser_store_error(Parser* self, ErrorCode code) {
    if (self->in_panic_mode) return;

    self->in_panic_mode = true;

    Token token = self->current;
    self->errors[self->error_count++] = (Error) {
        .code=code,
        .location=self->current.location,
        .arg=(token.type == TOKEN_EOF) ? SLICE("") : parser_token_repr(self, token),
        .path=self->path,
        .source=self->source,
        .function=SLICE(""),
    };
}


static int skip_space(Parser* self) {
    int count = 0;
    while (is_space(self->source[self->location.index+count]))
        ++count;
    return count;
}

static void skip_whitespace(Parser* self) {
    int count = 0;
    char c = self->source[self->location.index+count];
    while (c == ' ' || c == '\t' || c == '\n') {
        if (c == '\n') {
            self->location.row += 1;
            self->location.col  = 0;
        }
        ++self->location.col;
        c = self->source[self->location.index + (++count)];
    }
    self->location.index += count;
}

const char* parser_peek(Parser* self, int count) {
    return &self->source[self->location.index+count];
}

static void parser_next(Parser* self, int count, TokenType type) {
    self->previous = self->current;
    self->current = token_make(type, self->location, count);
    self->location.index += count;
    self->location.col   += count;
    skip_whitespace(self);
}

static void parser_string(Parser* self) {
    int count = 0;
    self->location.index += 1;  // @NOTE: Skip '"'
    self->location.col   += 1;
    char c = self->source[self->location.index+count];
    while (c != '"' && c != '\n' && c != '\0')
        c = self->source[self->location.index + (++count)];

    if (c == '\0' || c == '\n') {
        parser_store_error(self, PARSER_ERROR_UNEXPECTED_EOF);
        return;
    }

    parser_next(self, count, TOKEN_STRING);
    self->location.index += 1; // @NOTE: Skip '"'
    self->location.col   += 1;
}

static void parser_comment(Parser* self) {
    int count = 0;
    self->location.index += 2;  // @NOTE: Skip '//'
    self->location.col   += 2;
    char c = self->source[self->location.index+count];
    while (c != '\n' && c != '\0')
        c = self->source[self->location.index + (++count)];

    self->location.index += count;
    self->location.col   += count;

    // @NOTE: Skip '\n' but not '\0'.
    if (c == '\n') {
        self->location.index += 1;
        self->location.col    = 0;
        self->location.row   += 1;
    }
}

static void parser_digit(Parser* self) {
    int count = 1;
    while (is_digit(self->source[self->location.index+count])) ++count;
    if (self->source[self->location.index+count] == '.') {
        ++count;
        while (is_digit(self->source[self->location.index+count])) ++count;
        parser_next(self, count, TOKEN_F64);
    } else {
        parser_next(self, count, TOKEN_I64);
    }
}

static void parser_identifier(Parser* self) {
    int count = 1;  // @NOTE: Skip already checked value.
    char c = self->source[self->location.index+count];
    while (is_alpha(c) || is_digit(c) || c == '_')
        c = self->source[self->location.index + (++count)];

    parser_next(self, count, TOKEN_IDENTIFIER);
}

static void parser_end(Parser* self) {
    self->previous = self->current;
    self->current = token_make(TOKEN_EOF, self->location, 0);
}


void advance(Parser* self) {
    char current;
    retry: switch (current = self->source[self->location.index]) {
        case '+':  parser_next(self, 1, TOKEN_PLUS);        return;
        case '-':  parser_next(self, 1, TOKEN_MINUS);       return;
        case '*':  parser_next(self, 1, TOKEN_STAR);        return;
        case '(':  parser_next(self, 1, TOKEN_LEFT_PAREN);  return;
        case ')':  parser_next(self, 1, TOKEN_RIGHT_PAREN); return;
        case '{':  parser_next(self, 1, TOKEN_LEFT_BRACE);  return;
        case '}':  parser_next(self, 1, TOKEN_RIGHT_BRACE); return;
        case '"':  parser_string(self); return;
        case '/':  if (*parser_peek(self, 1) == '/') { parser_comment(self); goto retry; }       else parser_next(self, 1, TOKEN_SLASH);    return;
        case '!':  if (*parser_peek(self, 1) == '=')  parser_next(self, 2, TOKEN_BANG_EQUAL);    else parser_next(self, 1, TOKEN_BANG);     return;
        case '=':  if (*parser_peek(self, 1) == '=')  parser_next(self, 2, TOKEN_EQUAL_EQUAL);   else parser_next(self, 1, TOKEN_EQUAL);    return;
        case '<':  if (*parser_peek(self, 1) == '=')  parser_next(self, 2, TOKEN_LESS_EQUAL);    else parser_next(self, 1, TOKEN_LESS);     return;
        case '>':  if (*parser_peek(self, 1) == '=')  parser_next(self, 2, TOKEN_GREATER_EQUAL); else parser_next(self, 1, TOKEN_GREATER);  return;
        case ';':  parser_next(self, 1, TOKEN_END_STMT); return;
        case ',':  parser_next(self, 1, TOKEN_COMMA);    return;
        case '\0': parser_end(self);  return;
        case ' ':
        case '\n': skip_whitespace(self); goto retry;
        default:
            switch (current) {
                case 'f':
                    if (is_keyword(parser_peek(self, 1), "alse"))  { parser_next(self, 5, TOKEN_FALSE);  return; }
                    if (is_keyword(parser_peek(self, 1), "or"))    { parser_next(self, 3, TOKEN_FOR);    return; }
                    if (is_keyword(parser_peek(self, 1), "un"))    { parser_next(self, 3, TOKEN_FUN);    return; }
                    break;
                case 'a': if (is_keyword(parser_peek(self, 1), "nd"))    { parser_next(self, 3, TOKEN_AND);        return; } break;
                case 'o': if (is_keyword(parser_peek(self, 1), "r"))     { parser_next(self, 2, TOKEN_OR);         return; } break;
                case 't': if (is_keyword(parser_peek(self, 1), "rue"))   { parser_next(self, 4, TOKEN_TRUE);       return; } break;
                case 'p': if (is_keyword(parser_peek(self, 1), "rint"))  { parser_next(self, 5, TOKEN_PRINT_STMT); return; } break;
                case 'v': if (is_keyword(parser_peek(self, 1), "ar"))    { parser_next(self, 3, TOKEN_VAR);        return; } break;
                case 'i': if (is_keyword(parser_peek(self, 1), "f"))     { parser_next(self, 2, TOKEN_IF);         return; } break;
                case 'e': if (is_keyword(parser_peek(self, 1), "lse"))   { parser_next(self, 4, TOKEN_ELSE);       return; } break;
                case 'w': if (is_keyword(parser_peek(self, 1), "hile"))  { parser_next(self, 5, TOKEN_WHILE);      return; } break;
                case 'r': if (is_keyword(parser_peek(self, 1), "eturn")) { parser_next(self, 6, TOKEN_RETURN);     return; } break;
            }
            if (is_alpha(current) || current == '_') {
                parser_identifier(self);
                break;
            } else if (is_digit(current)) {
                parser_digit(self);
                break;
            }
            parser_unknown_token(self);
    }
}

void parser_unknown_token(Parser* self) {
    int count = 0;
    char c = self->source[self->location.index + count];
    while (!is_valid(c)) {
        c = self->source[self->location.index + (++count)];
    }

    parser_next(self, count, TOKEN_UNKNOWN);
    parser_store_error(self, PARSER_ERROR_UNKNOWN_TOKEN);
}

bool expect(Parser* self, TokenType type) {
    if (self->current.type == type) {
        advance(self);
        return true;
    } else {
        return false;
    }
}


Slice parser_token_repr(Parser* self, Token token) {
    return (Slice) { .source=&self->source[token.location.index], .count=token.count };
}



static inline bool is_space(char c) { return (c == ' ' || c == '\t'); }
static inline bool is_digit(char c) { return ('0' <= c && c <= '9'); }
static inline bool is_alpha(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
static inline bool is_valid(char c) {
    if (is_space(c) || is_alpha(c) || is_digit(c) || c == '_' || c == '\n')
        return true;
    switch (c) {
        case '+':
        case '-':
        case '*':
        case '(':
        case ')':
        case '{':
        case '}':
        case '"':
        case '/':
        case '!':
        case '=':
        case '<':
        case '>':
        case ';':
        case ',': return true;
        default:  return false;
    }
}
