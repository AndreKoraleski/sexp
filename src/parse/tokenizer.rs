/// A single lexical unit produced by [`Tokenizer`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub(super) enum Token<'input> {
    LeftParen,
    RightParen,
    /// An atom string, borrowed directly from the input.
    Atom(&'input str),
}

/// Lazily tokenizes an S-expression input string.
///
/// Yields [`Token`] values in order. Whitespace between tokens is silently consumed.  The tokenizer
/// itself never fails. The parser detects structural errors from the token stream.
pub(super) struct Tokenizer<'input> {
    remaining: &'input str,
}

impl<'input> Tokenizer<'input> {
    /// Creates a tokenizer over `input`.
    pub(super) fn new(input: &'input str) -> Self {
        Self { remaining: input }
    }
}

/// Per-byte classification table.
const STOP: [u8; 256] = {
    let mut t = [0u8; 256];
    t[b'(' as usize] = 1;
    t[b')' as usize] = 1;
    t[b' ' as usize] = 2;
    t[b'\t' as usize] = 2;
    t[b'\n' as usize] = 2;
    t[b'\r' as usize] = 2;
    t
};

impl<'input> Iterator for Tokenizer<'input> {
    type Item = Token<'input>;

    #[inline(always)]
    fn next(&mut self) -> Option<Token<'input>> {
        let bytes = self.remaining.as_bytes();

        // Skip leading whitespace: advance past every byte whose class is 2.
        // Class 0 (regular) and class 1 (bracket) are both < 2, so they stop the scan.
        let start = bytes
            .iter()
            .position(|&b| STOP[b as usize] < 2)
            .unwrap_or(bytes.len());

        if start == bytes.len() {
            self.remaining = &self.remaining[start..];
            return None;
        }

        match bytes[start] {
            b'(' => {
                self.remaining = &self.remaining[start + 1..];
                Some(Token::LeftParen)
            }
            b')' => {
                self.remaining = &self.remaining[start + 1..];
                Some(Token::RightParen)
            }
            _ => {
                let rest = &bytes[start..];
                // Find the end of the atom: the first byte that is a bracket (class 1) or
                // whitespace (class 2). Both have STOP != 0.
                let end = rest
                    .iter()
                    .position(|&b| STOP[b as usize] != 0)
                    .unwrap_or(rest.len());
                let atom = &self.remaining[start..start + end];
                self.remaining = &self.remaining[start + end..];
                Some(Token::Atom(atom))
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_input_yields_no_tokens() {
        let tokens: Vec<_> = Tokenizer::new("").collect();
        assert!(tokens.is_empty());
    }

    #[test]
    fn whitespace_only_input_yields_no_tokens() {
        let tokens: Vec<_> = Tokenizer::new("  \t\n\r  ").collect();
        assert!(tokens.is_empty());
    }

    #[test]
    fn parentheses_and_atom_tokenize_in_order() {
        let tokens: Vec<_> = Tokenizer::new("(hello world)").collect();
        assert_eq!(
            tokens,
            vec![
                Token::LeftParen,
                Token::Atom("hello"),
                Token::Atom("world"),
                Token::RightParen,
            ]
        );
    }

    #[test]
    fn atom_borrows_from_input() {
        let input = String::from("foo");
        let tokens: Vec<_> = Tokenizer::new(&input).collect();
        assert_eq!(tokens, vec![Token::Atom("foo")]);
    }

    #[test]
    fn nested_parens_tokenize_correctly() {
        let tokens: Vec<_> = Tokenizer::new("(a (b c))").collect();
        assert_eq!(
            tokens,
            vec![
                Token::LeftParen,
                Token::Atom("a"),
                Token::LeftParen,
                Token::Atom("b"),
                Token::Atom("c"),
                Token::RightParen,
                Token::RightParen,
            ]
        );
    }
}
