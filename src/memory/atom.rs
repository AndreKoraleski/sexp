use std::fmt;

/// An interned atom value.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Atom(AtomInner);

// `Inline` stores `len: u8` (1 byte) + `buffer: [u8; INLINE_CAP]` bytes.
// Setting INLINE_CAP = 15 makes the Inline variant exactly 16 bytes, which matches `Box<str>`
// (pointer + length = 2 × usize = 16 bytes on 64-bit targets).  Both arms of the enum are therefore
// the same size, so the compiler does not need to pad the smaller arm - the total enum size is 16
// bytes + 1 byte discriminant (+ alignment padding), the smallest it can be while still holding a
// heap pointer.
//
// The choice of 15 is therefore not arbitrary: bumping it to 16 would push Inline to 17 bytes and
// the whole enum to 24 bytes, wasting 7 bytes on every heap-allocated atom.
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

    /// Returns `true` if the atom is stored inline (stack-only, no heap allocation).
    /// Only used in tests to verify the inline/owned split at the capacity boundary.
    #[cfg(test)]
    fn is_inline(atom: &Atom) -> bool {
        matches!(atom.0, AtomInner::Inline { .. })
    }

    #[test]
    fn max_inline_length_is_stored_inline() {
        // A string of exactly INLINE_CAP bytes must NOT heap-allocate.
        let s = "a".repeat(INLINE_CAP);
        let atom = Atom::new(&s);
        assert!(is_inline(&atom), "expected inline storage for {}-byte atom", INLINE_CAP);
        assert_eq!(atom.as_str(), s);
    }

    #[test]
    fn one_over_inline_cap_is_heap_allocated() {
        // A string of INLINE_CAP + 1 bytes must spill to the heap.
        let s = "a".repeat(INLINE_CAP + 1);
        let atom = Atom::new(&s);
        assert!(!is_inline(&atom), "expected heap storage for {}-byte atom", INLINE_CAP + 1);
        assert_eq!(atom.as_str(), s);
    }

    #[test]
    fn inline_cap_boundary_roundtrip_preserves_content() {
        // Verify the full 15-byte inline buffer is read back correctly.
        let s = "abcdefghijklmno"; // exactly 15 bytes
        assert_eq!(s.len(), INLINE_CAP);
        let atom = Atom::new(s);
        assert!(is_inline(&atom));
        assert_eq!(atom.as_str(), s);
    }

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
