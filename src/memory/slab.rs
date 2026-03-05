use std::marker::PhantomData;
use thunderdome::{Arena, Index};

/// A typed, generational handle into a [`Slab<T>`].
///
/// A key becomes stale when the value it refers to is removed. Accessing a
/// stale key via [`Slab::get`] returns `None`, via indexing it panics.
pub struct Key<T>(Index, PhantomData<fn() -> T>);

impl<T> Clone for Key<T> {
    fn clone(&self) -> Self {
        *self
    }
}

impl<T> Copy for Key<T> {}

impl<T> PartialEq for Key<T> {
    fn eq(&self, other: &Self) -> bool {
        self.0 == other.0
    }
}

impl<T> Eq for Key<T> {}

impl<T> std::hash::Hash for Key<T> {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.0.hash(state);
    }
}

impl<T> std::fmt::Debug for Key<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Key({:?})", self.0)
    }
}

/// A generational arena keyed by [`Key<T>`].
pub struct Slab<T>(Arena<T>);

impl<T> Slab<T> {
    /// Creates an empty slab.
    pub fn new() -> Self {
        Self(Arena::new())
    }

    /// Creates an empty slab with at least the given capacity.
    pub fn with_capacity(initial_capacity: usize) -> Self {
        Self(Arena::with_capacity(initial_capacity))
    }

    /// Inserts a value and returns a key for it.
    pub fn insert(&mut self, value: T) -> Key<T> {
        Key(self.0.insert(value), PhantomData)
    }

    /// Removes the value at `key` and returns it, or `None` if the key is stale.
    pub fn remove(&mut self, key: Key<T>) -> Option<T> {
        self.0.remove(key.0)
    }

    /// Returns a reference to the value at `key`, or `None` if the key is stale.
    pub fn get(&self, key: Key<T>) -> Option<&T> {
        self.0.get(key.0)
    }

    /// Returns a mutable reference to the value at `key`, or `None` if the key is stale.
    pub fn get_mut(&mut self, key: Key<T>) -> Option<&mut T> {
        self.0.get_mut(key.0)
    }

    /// Returns the number of values in the slab.
    pub fn len(&self) -> usize {
        self.0.len()
    }

    /// Returns `true` if the slab contains no values.
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    /// Returns the number of values the slab can hold without reallocating.
    pub fn capacity(&self) -> usize {
        self.0.capacity()
    }
}

impl<T> Default for Slab<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Clone> Clone for Slab<T> {
    fn clone(&self) -> Self {
        Slab(self.0.clone())
    }
}

impl<T> std::ops::Index<Key<T>> for Slab<T> {
    type Output = T;

    /// # Panics
    ///
    /// Panics if `key` is stale.
    fn index(&self, key: Key<T>) -> &T {
        &self.0[key.0]
    }
}

impl<T> std::ops::IndexMut<Key<T>> for Slab<T> {
    /// # Panics
    ///
    /// Panics if `key` is stale.
    fn index_mut(&mut self, key: Key<T>) -> &mut T {
        &mut self.0[key.0]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn inserted_value_is_retrievable() {
        let mut slab: Slab<u32> = Slab::new();
        let key = slab.insert(42);
        assert_eq!(slab[key], 42);
        assert_eq!(slab.len(), 1);
    }

    #[test]
    fn removed_key_returns_none() {
        let mut slab: Slab<u32> = Slab::new();
        let key = slab.insert(1);
        slab.remove(key);
        assert!(slab.get(key).is_none());
    }

    #[test]
    fn removed_key_does_not_alias_reused_slot() {
        let mut slab: Slab<u32> = Slab::new();
        let first_key = slab.insert(1);
        slab.remove(first_key);
        let _second_key = slab.insert(2);
        assert!(slab.get(first_key).is_none());
    }

    #[test]
    fn copied_key_equals_original() {
        let mut slab: Slab<u32> = Slab::new();
        let key = slab.insert(0);
        let key_copy = key;
        assert_eq!(key, key_copy);
    }
}
