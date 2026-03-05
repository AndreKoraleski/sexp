use std::fmt;

/// Describes why a parse attempt failed.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ParseError {
    /// The input contains an opening parenthesis with no matching closing parenthesis.
    UnclosedParenthesis,

    /// The input contains a closing parenthesis with no preceding opening parenthesis.
    StrayClosingParenthesis,

    /// The input contains more than one top-level form.
    ///
    /// An S-expression is by definition a single form. Inputs like `"(a)(b)"` or `"a b"` are
    /// rejected. Wrap them in an outer list if a sequence is intended.
    MultipleTopLevelForms,

    /// The byte input is not valid UTF-8.
    InvalidUtf8,
}

impl fmt::Display for ParseError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ParseError::UnclosedParenthesis => formatter.write_str("unclosed parenthesis"),
            ParseError::StrayClosingParenthesis => formatter.write_str("stray closing parenthesis"),
            ParseError::MultipleTopLevelForms => formatter.write_str("multiple top-level forms"),
            ParseError::InvalidUtf8 => formatter.write_str("input is not valid UTF-8"),
        }
    }
}

impl std::error::Error for ParseError {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn display_matches_python_error_messages() {
        assert_eq!(
            ParseError::UnclosedParenthesis.to_string(),
            "unclosed parenthesis"
        );
        assert_eq!(
            ParseError::StrayClosingParenthesis.to_string(),
            "stray closing parenthesis"
        );
        assert_eq!(
            ParseError::MultipleTopLevelForms.to_string(),
            "multiple top-level forms"
        );
    }

    #[test]
    fn parse_error_implements_std_error() {
        fn accepts_error(_: &dyn std::error::Error) {}
        accepts_error(&ParseError::UnclosedParenthesis);
    }
}
