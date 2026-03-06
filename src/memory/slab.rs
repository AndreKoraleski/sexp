use std::marker::PhantomData;

/// A typed index into a [`Slab<T>`].
///
/// Internally a `u32` offset into the slot `Vec`. Valid as long as the slot
/// has not been freed and re-occupied by a different value.
pub struct Key<T>(u32, PhantomData<fn() -> T>);

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
        write!(f, "Key({})", self.0)
    }
}

/// A dense, reuse-capable arena keyed by [`Key<T>`].
///
/// Backed by a `Vec<Option<T>>` and a `Vec<u32>` free list.  Each access is a single array bounds
/// check plus an `Option` discriminant test.
pub struct Slab<T> {
    slots: Vec<Option<T>>,
    free: Vec<u32>,
}

impl<T> Slab<T> {
    /// Creates an empty slab.
    pub fn new() -> Self {
        Self {
            slots: Vec::new(),
            free: Vec::new(),
        }
    }

    /// Creates an empty slab with at least the given capacity.
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            slots: Vec::with_capacity(capacity),
            free: Vec::new(),
        }
    }

    /// Inserts a value and returns a key for it.
    pub fn insert(&mut self, value: T) -> Key<T> {
        if let Some(idx) = self.free.pop() {
            self.slots[idx as usize] = Some(value);
            Key(idx, PhantomData)
        } else {
            let idx = self.slots.len() as u32;
            self.slots.push(Some(value));
            Key(idx, PhantomData)
        }
    }

    /// Removes the value at `key` and returns it, or `None` if the key is stale.
    pub fn remove(&mut self, key: Key<T>) -> Option<T> {
        let slot = self.slots.get_mut(key.0 as usize)?;
        let value = slot.take()?;
        self.free.push(key.0);
        Some(value)
    }

    /// Returns a reference to the value at `key`, or `None` if the key is stale.
    pub fn get(&self, key: Key<T>) -> Option<&T> {
        self.slots.get(key.0 as usize)?.as_ref()
    }

    /// Returns a mutable reference to the value at `key`, or `None` if the key is stale.
    pub fn get_mut(&mut self, key: Key<T>) -> Option<&mut T> {
        self.slots.get_mut(key.0 as usize)?.as_mut()
    }

    /// Returns the number of live values in the slab.
    pub fn len(&self) -> usize {
        self.slots.len() - self.free.len()
    }

    /// Returns `true` if the slab contains no live values.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<T> Default for Slab<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Clone> Clone for Slab<T> {
    fn clone(&self) -> Self {
        Slab {
            slots: self.slots.clone(),
            free: self.free.clone(),
        }
    }
}

impl<T> std::ops::Index<Key<T>> for Slab<T> {
    type Output = T;

    /// # Panics
    ///
    /// Panics if `key` is stale or out of range.
    fn index(&self, key: Key<T>) -> &T {
        self.slots[key.0 as usize].as_ref().expect("stale key")
    }
}

impl<T> std::ops::IndexMut<Key<T>> for Slab<T> {
    /// # Panics
    ///
    /// Panics if `key` is stale or out of range.
    fn index_mut(&mut self, key: Key<T>) -> &mut T {
        self.slots[key.0 as usize].as_mut().expect("stale key")
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
    fn freed_slot_is_reused_and_old_key_aliases_new_value() {
        let mut slab: Slab<u32> = Slab::new();
        let first_key = slab.insert(1);
        slab.remove(first_key);
        let _second_key = slab.insert(2);
        assert_eq!(slab.get(first_key), Some(&2));
    }

    #[test]
    fn copied_key_equals_original() {
        let mut slab: Slab<u32> = Slab::new();
        let key = slab.insert(0);
        let key_copy = key;
        assert_eq!(key, key_copy);
    }
}
