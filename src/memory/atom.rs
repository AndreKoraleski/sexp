use std::fmt;

/// An interned atom value.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Atom(AtomInner);

// Inner representation is sealed.
#[derive(Clone, PartialEq, Eq, Hash)]
enum AtomInner {
    Owned(Box<str>),
}

impl Atom {
    /// Creates an atom from a string slice.
    pub fn new(value: &str) -> Self {
        Self(AtomInner::Owned(value.into()))
    }

    /// Returns the atom as a string slice.
    pub fn as_str(&self) -> &str {
        match &self.0 {
            AtomInner::Owned(string) => string,
        }
    }
}

impl fmt::Display for Atom {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

impl fmt::Debug for Atom {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Atom({:?})", self.as_str())
    }
}

impl From<&str> for Atom {
    fn from(value: &str) -> Self {
        Self::new(value)
    }
}

impl From<String> for Atom {
    fn from(value: String) -> Self {
        Self(AtomInner::Owned(value.into_boxed_str()))
    }
}

impl AsRef<str> for Atom {
    fn as_ref(&self) -> &str {
        self.as_str()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn equal_atoms_compare_as_equal() {
        assert_eq!(Atom::new("foo"), Atom::new("foo"));
        assert_ne!(Atom::new("foo"), Atom::new("bar"));
    }

    #[test]
    fn display_outputs_raw_string() {
        assert_eq!(Atom::new("hello").to_string(), "hello");
    }

    #[test]
    fn atom_created_from_owned_string() {
        let atom = Atom::from(String::from("owned"));
        assert_eq!(atom.as_str(), "owned");
    }

    #[test]
    fn equal_atoms_produce_equal_hashes() {
        use std::collections::HashSet;
        let mut set = HashSet::new();
        set.insert(Atom::new("x"));
        assert!(set.contains(&Atom::new("x")));
        assert!(!set.contains(&Atom::new("y")));
    }
}
