#include "parser.h"
#include <stdio.h>
#include <string.h>

#define is_keyword(s, x)  (memcmp(s, x, sizeof(x)-1) == 0 && !is_alpha(*((s) + sizeof(x)-1)))

static inline bool is_space(char c);
static inline bool is_digit(char c);
static inline bool is_alpha(char c);
static int  skip_space(Parser* self);
static void skip_whitespace(Parser* self);
static void parser_next(Parser* self, int count, TokenType type);
static void parser_string(Parser* self);
static void parser_comment(Parser* self);
static void parser_digit(Parser* self);
static void parser_end(Parser* self);


Parser parser_make(const char* source) {
    Parser parser;
    parser.source = source;
    parser.index  = 0;
    parser.row    = 1;
    parser.col    = 0;
    parser.prev   = (Token) { TOKEN_ERROR, 0, 0, 0, 0 };
    parser.curr   = (Token) { TOKEN_ERROR, 0, 0, 0, 0 };
    parser.had_error     = false;
    parser.in_panic_mode = false;
    return parser;
}


void parser_error(Parser* self, const char* message) {
    if (self->in_panic_mode) return;
    self->in_panic_mode = true;

    Token token = self->curr;

    fprintf(stderr, "[line %d] Error", self->row);

    if (token.type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token.type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token.count, &self->source[token.index]);
    }

    fprintf(stderr, ": %s\n", message);
    self->had_error = true;
}

static int skip_space(Parser* self) {
    int count = 0;
    while (is_space(self->source[self->index+count]))
        ++count;
    return count;
}

static void skip_whitespace(Parser* self) {
    int count = 0;
    char c = self->source[self->index+count];
    while (c == ' ' || c == '\t' || c == '\n') {
        if (c == '\n') {
            self->row += 1;
            self->col  = 0;
        }
        ++self->col;
        c = self->source[self->index + (++count)];
    }
    self->index += count;
}

const char* parser_peek(Parser* self, int count) {
    return &self->source[self->index+count];
}

static void parser_next(Parser* self, int count, TokenType type) {
    self->prev = self->curr;
    self->curr = (Token) { .type=type, .row=self->row, .col=self->col, .index=self->index, count };
    self->index += count;
    self->col   += count;
    skip_whitespace(self);
}

static void parser_string(Parser* self) {
    int count = 0;
    self->index += 1;  // @NOTE: Skip '"'
    self->col   += 1;
    char c = self->source[self->index+count];
    while (c != '"' && c != '\n' && c != '\0')
        c = self->source[self->index + (++count)];

    if (c == '\0' || c == '\n')
        parser_error(self, "Unexpected EOF");

    parser_next(self, count+1, TOKEN_STRING);  // @NOTE: Skip '"'
}

static void parser_comment(Parser* self) {
    int count = 0;
    self->index += 2;  // @NOTE: Skip '//'
    self->col   += 2;
    char c = self->source[self->index+count];
    while (c != '\n' && c != '\0')
        c = self->source[self->index + (++count)];

    self->index += count;
    self->col   += count;

    // @NOTE: Skip '\n' but not '\0'.
    if (c == '\n') {
        self->index += 1;
        self->col    = 0;
        self->row   += 1;
    }
}

static void parser_digit(Parser* self) {
    int count = 1;
    while (is_digit(self->source[self->index+count])) ++count;
    if (self->source[self->index+count] == '.') {
        ++count;
        while (is_digit(self->source[self->index+count])) ++count;
        parser_next(self, count, TOKEN_F64);
    } else {
        parser_next(self, count, TOKEN_I64);
    }
}

static void parser_identifier(Parser* self) {
    int count = 1;  // @NOTE: Skip already checked value.
    char c = self->source[self->index+count];
    while (is_alpha(c) || is_digit(c) || c == '_')
        c = self->source[self->index + (++count)];

    parser_next(self, count, TOKEN_IDENTIFIER);
}

static void parser_end(Parser* self) {
    self->prev = self->curr;
    self->curr = (Token) { .type=TOKEN_EOF, .row=self->row, .col=self->col, .index=self->index, .count=0 };
}


void advance(Parser* self) {
    char current;
    retry: switch (current = self->source[self->index]) {
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
        case '\0': parser_end(self);  return;
        case ' ':
        case '\n': skip_whitespace(self); goto retry;
        default:
            switch (current) {
                case 'f':
                    if (is_keyword(parser_peek(self, 1), "alse"))  { parser_next(self, 5, TOKEN_FALSE);  return; }
                    if (is_keyword(parser_peek(self, 1), "or"))    { parser_next(self, 3, TOKEN_FOR);    return; }
                    break;
                case 'a': if (is_keyword(parser_peek(self, 1), "nd"))    { parser_next(self, 3, TOKEN_AND);        return; } break;
                case 'o': if (is_keyword(parser_peek(self, 1), "r"))     { parser_next(self, 2, TOKEN_OR);         return; } break;
                case 't': if (is_keyword(parser_peek(self, 1), "rue"))   { parser_next(self, 4, TOKEN_TRUE);       return; } break;
                case 'p': if (is_keyword(parser_peek(self, 1), "rint"))  { parser_next(self, 5, TOKEN_PRINT_STMT); return; } break;
                case 'v': if (is_keyword(parser_peek(self, 1), "ar"))    { parser_next(self, 3, TOKEN_VAR);        return; } break;
                case 'i': if (is_keyword(parser_peek(self, 1), "f"))     { parser_next(self, 2, TOKEN_IF);         return; } break;
                case 'e': if (is_keyword(parser_peek(self, 1), "lse"))   { parser_next(self, 4, TOKEN_ELSE);       return; } break;
                case 'w': if (is_keyword(parser_peek(self, 1), "hile"))  { parser_next(self, 5, TOKEN_WHILE);      return; } break;
            }
            if (is_alpha(current) || current == '_') {
                parser_identifier(self);
                break;
            } else if (is_digit(current)) {
                parser_digit(self);
                break;
            }
            parser_error(self, "Unexpected token");
    }
}

bool expect(Parser* self, TokenType type) {
    if (self->curr.type == type) {
        advance(self);
        return true;
    } else {
        return false;
    }
}






static inline bool is_space(char c) { return (c == ' ' || c == '\t'); }
static inline bool is_digit(char c) { return ('0' <= c && c <= '9'); }
static inline bool is_alpha(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
