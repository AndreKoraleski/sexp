use std::fmt;

/// An interned atom value.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Atom(AtomInner);

const INLINE_CAP: usize = 15;

// Inner representation is sealed.
#[derive(Clone, PartialEq, Eq, Hash)]
enum AtomInner {
    Inline { len: u8, buffer: [u8; INLINE_CAP] },
    Owned(Box<str>),
}

impl Atom {
    /// Creates an atom from a string slice.
    pub fn new(value: &str) -> Self {
        if value.len() <= INLINE_CAP {
            let mut buffer = [0u8; INLINE_CAP];
            buffer[..value.len()].copy_from_slice(value.as_bytes());
            Self(AtomInner::Inline {
                len: value.len() as u8,
                buffer,
            })
        } else {
            Self(AtomInner::Owned(value.into()))
        }
    }

    /// Returns the atom as a string slice.
    pub fn as_str(&self) -> &str {
        match &self.0 {
            AtomInner::Inline { len, buffer: buf } => unsafe {
                std::str::from_utf8_unchecked(&buf[..*len as usize])
            },
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
        if value.len() <= INLINE_CAP {
            Self::new(&value)
        } else {
            Self(AtomInner::Owned(value.into_boxed_str()))
        }
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
