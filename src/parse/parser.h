#pragma once

#include <stddef.h>

#include "core/node.h"

/**
 * @brief Parses an S-expression from a buffer, returning a new tree.
 *
 * Retains a reference to the global intern pool automatically.
 *
 * @param source        Pointer to the input buffer.
 * @param source_length Length of the input buffer in bytes.
 * @return              A parsed SExp, or a zeroed struct on failure.
 */
SExp sexp_parse(const char *source, size_t source_length);
