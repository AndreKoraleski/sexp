#pragma once

#include <stddef.h>

/**
 * @brief Token categories produced by the tokenizer.
 */
typedef enum TokenKind {
    TOKEN_LEFT_PARENTHESIS,  /**< Opening parenthesis. */
    TOKEN_RIGHT_PARENTHESIS, /**< Closing parenthesis. */
    TOKEN_ATOM,              /**< Bare atom (identifier). */
    TOKEN_END,               /**< End of input. */
    TOKEN_ERROR,             /**< Unrecognised character. */
} TokenKind;

/**
 * @brief A single token with its kind, source pointer, and byte length.
 */
typedef struct Token {
    TokenKind   kind;   /**< Category of the token. */
    const char *string; /**< Pointer into the source string; NULL for non-atom tokens. */
    size_t      length; /**< Byte length of the token text. */
} Token;

/**
 * @brief Cursor state for the hand-written tokenizer.
 */
typedef struct Tokenizer {
    const char *cursor; /**< Current read position. */
    const char *end;    /**< One past the last byte of input. */
} Tokenizer;

/**
 * @brief Returns the next token from the tokenizer, advancing the cursor.
 *
 * @param tokenizer Pointer to the tokenizer state.
 * @return          The next token, or TOKEN_END if at end of input, or TOKEN_ERROR if an
 * unrecognised character is encountered.
 */
Token next_token(Tokenizer *tokenizer);
