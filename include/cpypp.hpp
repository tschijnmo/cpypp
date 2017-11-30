/** @file cpypp.hpp
 *
 * A simple wrapper of CPython C API for C++
 *
 * This is only header file for the project.  Everything is defined inside the
 * `cpypp` namespace.  It can simply be included in files where it is needed.
 */

#ifndef CPYPP_CPYPP_HPP
#define CPYPP_CPYPP_HPP

#include <algorithm>

namespace cpypp {

/** C++ exception signalling that a Python exception has been set.
 *
 * Inside a C++ function using cpypp, after a Python exception occurs and is
 * correctly registered on the Python runtime, this exception can be thrown to
 * signal the occurrence of a Python exception and its correct setting on the
 * Python stack.  Then on a higher level, likely the top-level C++ function,
 * this exception should be caught and handled by the canonical CPython way of
 * returning a special value, like a null pointer.
 *
 * When used in together with `Handle`, explicit handling of the Python object
 * reference counts are no longer necessary.
 *
 * Since the information about the actual error should be set on the Python
 * runtime as Python exception, this C++ exception generally contains no
 * information at all.
 */

class Exc_set {
};

/** Handles for Python objects.
 *
 * This base class serves as a handle to a Python object.  It can manage Python
 * reference counts automatically and expose some Python object protocols.
 *
 * The handle can be either borrowing or owning.  For borrowing handles, the
 * reference count of the underlying object is not touched.  For owning
 * handles, the reference count will be automatically decremented when the
 * handle no longer take the reference.  For example, when it is destructed.
 */

class Handle {
public:
    //
    // Special methods.
    //

    /** Constructs a handle for an object.
     *
     * @param ref The pointer to the Python object.  When a handle owning the
     * reference is to be created, this constructor *steals the reference* to
     * the object.
     *
     * @param if_borrow If this handle borrows the reference only.
     *
     * @param allow_null If null pointer triggers `Exc_set` to be thrown
     * immediately.  This can be useful for calling CPython functions that set
     * the exception correctly before returning a NULL.
     */

    Handle(PyObject* ref, bool if_borrow = false, bool allow_null = false)
        : ref_{ ref }
        , if_borrow_{ if_borrow }
    {
        if (ref == nullptr && !allow_null) {
            throw Exc_set();
        }
    }

    /** Constructs an empty handle by default.
     */

    Handle()
        : ref_{ nullptr }
        , if_borrow_{ true }
    {
    }

    /** Destructs the handle.
     *
     * For non-borrowed references and non-null pointers, the Python reference
     * count will be decremented.
     */

    ~Handle() { decr_ref(); }

    /** Constructs the handle by copying from another one.
     *
     * Note that borrowed handles also gives borrowed handles.
     */

    Handle(const Handle& other)
        : ref_{ other.ref_ }
        , if_borrow_{ other.if_borrow_ }
    {
        incr_ref();
    }

    /** Constructs the handle by moving from another one.
     */

    Handle(Handle&& other)
        : ref_{ other.ref_ }
        , if_borrow_{ other.if_borrow_ }
    {
        other.release();
    }

    /** Makes assignment from other object handle.
     *
     * Note that handles with borrowed references gives handles with borrowed
     * reference.
     */

    Handle& operator=(const Handle& other) noexcept
    {
        decr_ref();

        ref_ = other.ref_;
        if_borrow_ = other.if_borrow_;
        incr_ref();

        return *this;
    }

    /** Makes assignment by moving from other object handle.
     */

    Handle& operator=(Handle&& other) noexcept
    {
        decr_ref();

        if_borrow_ = other.if_borrow();
        ref_ = other.release();

        return *this;
    }

    /** If the handle only holds a borrowed reference.
     */

    bool if_borrow() const noexcept { return if_borrow_; }

    /** If the handle contains any object.
     */

    explicit operator bool() const noexcept { return ref_ != nullptr; }

    /** Resets the handle to refer to another Python object.
     *
     * All the parameters has the same semantics as the constructor.
     */

    void reset(PyObject* ref, bool if_borrow = false, bool allow_null = false)
    {
        decr_ref();

        ref_ = ref;
        if_borrow_ = if_borrow;

        if (ref == nullptr && !allow_null) {
            throw Exc_set();
        }
    }

    /** Gets the pointer to the Python object handled.
     *
     * This method simply returns the pointer and nothing is performed on the
     * ownership.
     */

    PyObject* get() const noexcept { return ref_; }

    /** Swaps the managed Python object with another handle.
     */

    void swap(Handle& other) noexcept
    {
        std::swap(ref_, other.ref_);
        std::swap(if_borrow_, other.if_borrow_);
    }

    /** Releases the ownership of the managed object.
     *
     * If no object is handled, a null pointer is going to be returned.  Note
     * that this method also just returns the management object for borrowing
     * handles without any special treatment.
     */

    PyObject* release() noexcept
    {
        auto ref = ref_;
        ref_ = nullptr;
        if_borrow_ = true;
        return ref;
    }

private:
    //
    // Reference counting handling.
    //

    /** Decrements the reference count for owning reference to a non-null.
     */

    void decr_ref()
    {
        if (!if_borrow_) {
            Py_XDECREF(ref_);
        }
    }

    /** Increments the reference count only for owning reference.
     */

    void incr_ref()
    {
        if (!if_borrow_) {
            Py_INCREF(ref_);
        }
    }

    //
    // Data fields.
    //

    /** The pointer to the actual Python object.
     */

    PyObject* ref_;

    /** If this is an borrowed reference.
     */

    bool if_borrow_;
};
}

#endif