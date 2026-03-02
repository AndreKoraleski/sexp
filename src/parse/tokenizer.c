#include "parse/tokenizer.h"

/**
 * @brief Returns non-zero if character is an ASCII whitespace character (space, tab, CR, or LF).
 *
 * @param character Character to classify.
 * @return int      Non-zero if character is whitespace, zero otherwise.
 */
static int is_whitespace(char character) {
    return character == ' ' || character == '\t' || character == '\n' || character == '\r';
}

/**
 * @brief Returns non-zero if character is a valid atom character (not whitespace and not a
 * parenthesis).
 *
 * @param character Character to classify.
 * @return int      Non-zero if character is a valid atom character, zero otherwise.
 */
static int is_atom_char(char character) {
    return !is_whitespace(character) && character != '(' && character != ')';
}

Token next_token(Tokenizer *tokenizer) {
    while (tokenizer->cursor < tokenizer->end && is_whitespace(*tokenizer->cursor)) {
        tokenizer->cursor++;
    }

    if (tokenizer->cursor >= tokenizer->end) {
        return (Token){TOKEN_END, NULL, 0};
    }

    char character = *tokenizer->cursor;

    if (character == '(') {
        tokenizer->cursor++;
        return (Token){TOKEN_LEFT_PARENTHESIS, NULL, 0};
    }

    if (character == ')') {
        tokenizer->cursor++;
        return (Token){TOKEN_RIGHT_PARENTHESIS, NULL, 0};
    }

    if (is_atom_char(character)) {
        const char *start = tokenizer->cursor;
        while (tokenizer->cursor < tokenizer->end && is_atom_char(*tokenizer->cursor)) {
            tokenizer->cursor++;
        }
        return (Token){TOKEN_ATOM, start, (size_t)(tokenizer->cursor - start)};
    }

    return (Token){TOKEN_ERROR, NULL, 0};
}
