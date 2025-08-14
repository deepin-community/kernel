// SPDX-License-Identifier: GPL-2.0

//! Untrusted data API.
//!
//! # Overview
//!
//! Untrusted data is marked using the [`Untrusted<T>`] type. See [Rationale](#rationale) for the
//! reasons to mark untrusted data throughout the kernel. It is a totally opaque wrapper, it is not
//! possible to read the data inside.
//!
//! APIs that write back into userspace usually allow writing untrusted bytes directly, allowing
//! direct copying of untrusted user data back into userspace without validation.
//!
//! The only way to access untrusted data is to [`Validate::validate`] it. This is facilitated by
//! the [`Validate`] trait.
//!
//! # Rationale
//!
//! When reading data from an untrusted source, it must be validated before it can be used for
//! **logic**. For example, this is a very bad idea:
//!
//! ```
//! # fn read_bytes_from_network() -> KBox<[u8]> {
//! #     Box::new([1, 0], kernel::alloc::flags::GFP_KERNEL).unwrap()
//! # }
//! let bytes: KBox<[u8]> = read_bytes_from_network();
//! let data_index = bytes[0];
//! let data = bytes[usize::from(data_index)];
//! ```
//!
//! While this will not lead to a memory violation (because the array index checks the bounds), it
//! might result in a kernel panic. For this reason, all untrusted data must be wrapped in
//! [`Untrusted<T>`]. This type only allows validating the data or passing it along, since copying
//! data from userspace back into userspace is allowed for untrusted data.

use core::ops::{Deref, DerefMut};

use crate::{
    alloc::{Allocator, Vec},
    transmute::{cast_slice, cast_slice_mut},
};

/// Untrusted data of type `T`.
///
/// Data coming from userspace is considered untrusted and should be marked by this type.
///
/// The particular meaning of [`Untrusted<T>`] depends heavily on the type `T`. For example,
/// `&Untrusted<[u8]>` is a reference to an untrusted slice. But the length is not considered
/// untrusted, as it would otherwise violate normal Rust rules. For this reason, one can easily
/// convert that reference to `&[Untrusted<u8>]`. Another such example is `Untrusted<KVec<T>>`, it
/// derefs to `KVec<Untrusted<T>>`. Raw bytes however do not behave in this way, `Untrusted<u8>` is
/// totally opaque and one can only access its value by calling [`Untrusted::validate()`].
///
/// # Usage in API Design
///
/// The exact location where to put [`Untrusted`] depends on the kind of API. When asking for an
/// untrusted input value, or buffer to write to, always move the [`Untrusted`] wrapper as far
/// inwards as possible:
///
/// ```ignore
/// // use this
/// pub fn read_from_userspace(buf: &mut [Untrusted<u8>]) { todo!() }
///
/// // and not this
/// pub fn read_from_userspace(buf: &mut Untrusted<[u8]>) { todo!() }
/// ```
///
/// The reason for this is that `&mut Untrusted<[u8]>` can beconverted into `&mut [Untrusted<u8>]`
/// very easily, but the converse is not possible.
///
/// For the same reason, when returning untrusted data by-value, one should move the [`Untrusted`]
/// wrapper as far outward as possible:
///
/// ```ignore
/// // use this
/// pub fn read_all_from_userspace() -> Untrusted<KVec<u8>> { todo!() }
///
/// // and not this
/// pub fn read_all_from_userspace() -> KVec<Untrusted<u8>> { todo!() }
/// ```
///
/// Here too the reason is that `KVec<Untrusted<u8>>` is more restrictive compared to
/// `Untrusted<KVec<u8>>`.
#[repr(transparent)]
pub struct Untrusted<T: ?Sized>(T);

impl<T: ?Sized> Untrusted<T> {
    /// Marks the given value as untrusted.
    ///
    /// # Examples
    ///
    /// ```
    /// use kernel::validate::Untrusted;
    ///
    /// # mod bindings { pub(crate) unsafe fn read_foo_info() -> [u8; 4] { todo!() } };
    /// fn read_foo_info() -> Untrusted<[u8; 4]> {
    ///     // SAFETY: just an FFI call without preconditions.
    ///     Untrusted::new(unsafe { bindings::read_foo_info() })
    /// }
    /// ```
    pub fn new(value: T) -> Self
    where
        T: Sized,
    {
        Self(value)
    }

    /// Validate the underlying untrusted data.
    ///
    /// See the [`Validate`] trait for more information.
    pub fn validate<V: Validate<Self>>(self) -> Result<V, V::Err>
    where
        T: Sized,
    {
        V::validate(self.0)
    }

    /// Validate the underlying untrusted data.
    ///
    /// See the [`Validate`] trait for more information.
    pub fn validate_ref<'a, V: Validate<&'a Self>>(&'a self) -> Result<V, V::Err> {
        V::validate(&self.0)
    }

    /// Validate the underlying untrusted data.
    ///
    /// See the [`Validate`] trait for more information.
    pub fn validate_mut<'a, V: Validate<&'a mut Self>>(&'a mut self) -> Result<V, V::Err> {
        V::validate(&mut self.0)
    }
}

impl<T> Deref for Untrusted<[T]> {
    type Target = [Untrusted<T>];

    fn deref(&self) -> &Self::Target {
        // SAFETY: `Untrusted<T>` transparently wraps `T`.
        unsafe { cast_slice(&self.0) }
    }
}

impl<T> DerefMut for Untrusted<[T]> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        // SAFETY: `Untrusted<T>` transparently wraps `T`.
        unsafe { cast_slice_mut(&mut self.0) }
    }
}

impl<T, A: Allocator> Deref for Untrusted<Vec<T, A>> {
    type Target = Vec<Untrusted<T>, A>;

    fn deref(&self) -> &Self::Target {
        let ptr: *const Untrusted<Vec<T, A>> = self;
        // CAST: `Untrusted<T>` transparently wraps `T`.
        let ptr: *const Vec<Untrusted<T>, A> = ptr.cast();
        // SAFETY: `ptr` is derived from the reference `self`.
        unsafe { &*ptr }
    }
}

impl<T, A: Allocator> DerefMut for Untrusted<Vec<T, A>> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        let ptr: *mut Untrusted<Vec<T, A>> = self;
        // CAST: `Untrusted<T>` transparently wraps `T`.
        let ptr: *mut Vec<Untrusted<T>, A> = ptr.cast();
        // SAFETY: `ptr` is derived from the reference `self`.
        unsafe { &mut *ptr }
    }
}

/// Marks valid input for the [`Validate`] trait.
pub trait ValidateInput: private::Sealed {
    /// Type of the inner data.
    type Inner: ?Sized;
}

impl<T: ?Sized> ValidateInput for Untrusted<T> {
    type Inner = T;
}

impl<'a, T: ?Sized> ValidateInput for &'a Untrusted<T> {
    type Inner = &'a T;
}

impl<'a, T: ?Sized> ValidateInput for &'a mut Untrusted<T> {
    type Inner = &'a mut T;
}

mod private {
    use super::Untrusted;

    pub trait Sealed {}

    impl<T: ?Sized> Sealed for Untrusted<T> {}
    impl<'a, T: ?Sized> Sealed for &'a Untrusted<T> {}
    impl<'a, T: ?Sized> Sealed for &'a mut Untrusted<T> {}
}

/// Validate [`Untrusted`] data.
///
/// Care must be taken when implementing this trait, as unprotected access to unvalidated data is
/// given to the [`Validate::validate`] function. The implementer must ensure that the data is only
/// used for logic after successful validation.
pub trait Validate<Input: ValidateInput>: Sized {
    /// Validation error.
    type Err;

    /// Validate the raw input.
    fn validate(raw: Input::Inner) -> Result<Self, Self::Err>;
}
