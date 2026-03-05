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

/// Returns `true` for the four ASCII whitespace characters the tokenizer skips.
fn is_whitespace(character: char) -> bool {
    matches!(character, ' ' | '\t' | '\n' | '\r')
}

/// Returns `true` for any character that may appear inside an atom token.
fn is_atom_char(character: char) -> bool {
    !is_whitespace(character) && character != '(' && character != ')'
}

impl<'input> Iterator for Tokenizer<'input> {
    type Item = Token<'input>;

    fn next(&mut self) -> Option<Token<'input>> {
        // Skip leading whitespace.
        let start = self
            .remaining
            .find(|c: char| !is_whitespace(c))
            .unwrap_or(self.remaining.len());
        self.remaining = &self.remaining[start..];

        let first = self.remaining.chars().next()?;

        match first {
            '(' => {
                self.remaining = &self.remaining[1..];
                Some(Token::LeftParen)
            }
            ')' => {
                self.remaining = &self.remaining[1..];
                Some(Token::RightParen)
            }
            _ => {
                let end = self
                    .remaining
                    .find(|c: char| !is_atom_char(c))
                    .unwrap_or(self.remaining.len());
                let atom = &self.remaining[..end];
                self.remaining = &self.remaining[end..];
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
