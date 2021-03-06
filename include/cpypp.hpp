/** @file cpypp.hpp
 *
 * A simple wrapper of CPython C API for C++
 *
 * This is only header file for the project.  Everything is defined inside the
 * `cpypp` namespace.  It can simply be included in files where it is needed.
 *
 * The both the contents of this file and their tests are roughly sectioned in
 * the same way as the organization of the CPython C API documentation.
 */

#ifndef CPYPP_CPYPP_HPP
#define CPYPP_CPYPP_HPP

#include <algorithm>
#include <cassert>
#include <utility>

#include <Python.h>

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

/** Checks if error has occurred on the Python stack.
 *
 * This function throws the C++ `Exc_set` exception when a Python exception is
 * detected to have occurred.  In the absence of an Python exception, a value
 * of zero is returned.
 */

inline int check_exc()
{
    if (PyErr_Occurred()) {
        throw Exc_set();
    }
    return 0;
}

//
// Forward declaration of some types.
//

class Iter_handle;

/** Ways to take the given Python object reference by Handle.
 *
 * `STEAL` assumes that the given pointer is a new reference, which will be
 * stolen.
 *
 * `BORROW` only creates a borrowing handle, which does not touch the reference
 * count of the managed object.
 *
 * `NEW` takes a borrowed reference but create a new reference for the handle.
 */

enum Own { STEAL, BORROW, NEW };

/** Handles for Python objects.
 *
 * This base class serves as a handle to a Python object.  It can manage Python
 * reference counts automatically and expose some Python object operations.
 *
 * The handle can be either borrowing or owning.  For borrowing handles, the
 * reference count of the underlying object is not touched.  For owning
 * handles, the reference count will be automatically decremented when the
 * handle no longer take the reference.  For example, when it is destructed.
 */

class Handle {
public:
    //
    // Special methods
    //

    /** Constructs a handle for an object.
     *
     * @param ref The pointer to the Python object.  By default, it *steals the
     * reference* to the object.
     *
     * @param own How the ownership of the reference is to be treated, as
     * explained in `Own`.
     *
     * @param allow_null If null pointer triggers `Exc_set` to be thrown
     * immediately.  This can be useful for calling CPython functions that set
     * the exception correctly before returning a NULL.
     */

    Handle(PyObject* ref, Own own = STEAL, bool allow_null = false)
    {
        set(ref, own, allow_null);
    }

    /** Constructs an empty handle by default.
     */

    Handle() noexcept
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

    Handle(const Handle& other) noexcept
        : ref_{ other.ref_ }
        , if_borrow_{ other.if_borrow_ }
    {
        incr_ref();
    }

    /** Constructs the handle by moving from another one.
     *
     * Note that the handled that is moved from is not cleared to be empty, but
     * rather turned into a borrowing handle if it is not a borrowing one
     * already.
     */

    Handle(Handle&& other) noexcept
        : ref_{ other.ref_ }
        , if_borrow_{ other.if_borrow_ }
    {
        other.if_borrow_ = true;
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
     *
     * Similar to move initialization, here the handle that is moved from is
     * only turned into a borrowing handle, rather than cleared out.
     */

    Handle& operator=(Handle&& other) noexcept
    {
        decr_ref();

        if_borrow_ = other.if_borrow();
        ref_ = other.get();

        other.if_borrow_ = true;

        return *this;
    }

    //
    // Methods querying basic info about the current handle
    //

    /** If the handle contains any object.
     */

    explicit operator bool() const noexcept { return ref_ != nullptr; }

    /** If the handle only holds a borrowed reference.
     */

    bool if_borrow() const noexcept { return if_borrow_; }

    /** Gets the pointer to the Python object handled.
     *
     * This method simply returns the pointer and nothing is performed on the
     * ownership.
     */

    PyObject* get() const noexcept { return ref_; }

    /** Casts to a raw Python object pointer.
     *
     * This can be helpful for working with CPython API functions or even cpypp
     * functions taking a borrowed reference to a Python object just as input.
     *
     * Note that it will cause serious error if the result is used for
     * functions stealing references!
     */

    operator PyObject*() const noexcept { return get(); }

    /** Compares the identity of the underlying object.
     *
     * Because of the implicit cast rules, this method can be used for the
     * comparison with either handles or raw pointers.
     */

    bool is(const PyObject* o) const noexcept { return get() == o; }

    /** Gets the handled pointer with a new reference created.
     *
     * This method returns the pointer to the underlying object and increments
     * its reference count for non-empty handles.  This can be useful for
     * working with functions stealing references from its arguments.
     *
     * This method can also be called as a function, which is especially useful
     * to take references from R-values of Handles.
     */

    PyObject* get_new() const noexcept
    {
        if (ref_ != nullptr) {
            Py_INCREF(ref_);
        }
        return ref_;
    }

    /** Releases the ownership of the managed object.
     *
     * If no object is handled, a null pointer is going to be returned.  For
     * owning handles, a new reference to the handled object is going to be
     * returned, with the handle itself turned into an borrowing handle to the
     * same object.  For borrowing handles, the handle will not be touched,
     * just a new reference is to be returned.
     *
     * Note that whenever the handle is not empty, a new reference is always
     * returned by this method.  Compared with `get_new` method, this method
     * can be used when the handle no longer need to own a reference.  At the
     * same time, the `get_new` non-member function can dispatch to either this
     * method or `get_new` method differently for Handle R-values and L-values.
     */

    PyObject* release() noexcept
    {
        if (ref_ != nullptr) {
            if (if_borrow_) {
                Py_INCREF(ref_);
            } else {
                if_borrow_ = true;
            }
        }
        return ref_;
    }

    //
    // Methods for mutating the object handled
    //

    /** Resets the handle to refer to another Python object.
     *
     * All the parameters has the same semantics as the constructor.
     */

    void reset(PyObject* ref, Own own = STEAL, bool allow_null = false)
    {
        decr_ref();
        set(ref, own, allow_null);
    }

    /** Gets a pointer to PyObject pointer to read in a borrowed reference.
     *
     * This method is for the convenience of working with CPython API functions
     * like PyArg_ParseTuple.  The returned pointer can be directly given to
     * this kind of CPython API functions to read the object to be handled in.
     * Note that this method can only be used to create borrowing references.
     */

    PyObject** read() noexcept
    {
        decr_ref();
        ref_ = nullptr;
        if_borrow_ = true;
        return &ref_;
    }

    /** Swaps the managed Python object with another handle.
     */

    void swap(Handle& other) noexcept
    {
        std::swap(ref_, other.ref_);
        std::swap(if_borrow_, other.if_borrow_);
    }

    //
    // Utilities for building and parsing values
    //
    // Here we have the generic utilities for building/parsing Python objects
    // from/into native C++ values.  Individual concrete Python object types
    // may also have specialized methods for these two purposes.
    //
    // Normally, to build from native C++ types, we have an overload of the
    // constructor.  For some special types like tuple, we also have subclass
    // of this Handle class for their construction.
    //
    // To read into a C++ type, we can have an `as` overload, which reads the
    // Python object into the given C++ native object.  For all these
    // overloads, the target is put as an reference argument so that the
    // overload can be automatically deduced.  When the conversion fails, the
    // `Exc_set` exception will be thrown.  With the `as` template, these
    // overload can also automatically be used to generate R-values.
    //

    /** Constructs a handle from result of the Py_BuildValue function.
     *
     * All the arguments are exactly forwarded to the CPython Py_BuildValue
     * function, with the resulted objected given to the handle.  `Exc_set`
     * will be raised if the construction failed.
     */

    template <typename... Args>
    Handle(const char* format, Args&&... args)
        : Handle(Py_BuildValue(format, std::forward<Args>(args)...))
    {
    }

    /** Reads the Python object as return value.
     *
     * This tiny wrapper over the normal `as` methods gives an alternative
     * interface for reading a Python object into a native C++ value.  Rather
     * than taking an l-value reference, a pr-value of the given type is
     * returned.  Error is likewise handled by `Exc_set` C++ exception.
     *
     * Note that the type of the Handle object might need to be explicitly
     * declared to be `Handle` (rather than auto) when this function is used
     * inside templates.
     */

    template <typename T> T as() const
    {
        T res;
        as(res);
        return res;
    }

    //
    // Object protocol
    //

    // Attribute manipulations.

    /** Gets an attribute for the object.
     *
     * Exception will be set when failure occurs during getting the attribute.
     */

    Handle getattr(const char* attr)
    {
        return Handle{ PyObject_GetAttrString(get(), attr) };
    }

    /** Sets an attribute for the handled object.
     *
     * This methods takes an non-owning reference to the object to be set as
     * the given attribute, which can be from implicit casting from a Handle.
     */

    void setattr(const char* attr, PyObject* v)
    {
        if (PyObject_SetAttrString(get(), attr, v) != 0) {
            throw Exc_set{};
        }
        return;
    }

    /** Deletes the given attributes.
     */

    void delattr(const char* attr)
    {
        if (PyObject_DelAttrString(get(), attr) != 0) {
            throw Exc_set{};
        }
        return;
    }

    // Python object comparisons
    //
    // The C++ comparison operators are all mapped into the corresponding
    // comparison for Python object.

    bool operator<(const Handle& o) const { return compare<Py_LT>(o); }
    bool operator<=(const Handle& o) const { return compare<Py_LE>(o); }
    bool operator==(const Handle& o) const { return compare<Py_EQ>(o); }
    bool operator!=(const Handle& o) const { return compare<Py_NE>(o); }
    bool operator>(const Handle& o) const { return compare<Py_GT>(o); }
    bool operator>=(const Handle& o) const { return compare<Py_GE>(o); }

    //
    // Number protocol
    //

    /** Checks if the object provide numeric protocols.
     */

    bool check_number() const noexcept { return PyNumber_Check(ref_) == 1; }

    /** Adds two numbers.
     */

    friend Handle operator+(const Handle& o1, const Handle& o2)
    {
        return { PyNumber_Add(o1.get(), o2.get()) };
    }

    /** Substracts one number from other.
     */

    friend Handle operator-(const Handle& o1, const Handle& o2)
    {
        return { PyNumber_Subtract(o1.get(), o2.get()) };
    }

    /** Multiplies two numbers.
     */

    friend Handle operator*(const Handle& o1, const Handle& o2)
    {
        return { PyNumber_Multiply(o1.get(), o2.get()) };
    }

    /** Divides by a number.
     *
     * Note that here we perform the floor division with `/` operator for less
     * astonishment for C++ coders.
     */

    friend Handle operator/(const Handle& o1, const Handle& o2)
    {
        return { PyNumber_FloorDivide(o1.get(), o2.get()) };
    }

    /** Gets the remainder.
     *
     * Note that the remainder operator in Python has slightly different
     * methatical definition than that in native C++.
     */

    friend Handle operator%(const Handle& o1, const Handle& o2)
    {
        return { PyNumber_Remainder(o1.get(), o2.get()) };
    }

    /** Gets the quotient and the remainder.
     */

    std::pair<Handle, Handle> divmod(const Handle& o) const
    {
        PyObject* res = PyNumber_Divmod(get(), o.get());
        if (res == nullptr) {
            throw Exc_set();
        }

        std::pair<Handle, Handle> ret{ Handle(PySequence_GetItem(res, 0)),
            Handle(PySequence_GetItem(res, 1)) };
        Py_DECREF(res);

        return ret;
    }

    //
    // Sequence protocol
    //

    //
    // Mapping protocol
    //

    //
    // Iterator protocol
    //

    /** Gets the iterator for iterating over the managed object.
     *
     * This method calls the `PyObject_GetIter` function and puts the resulted
     * Python iterator to be handled by the `Iter_handle` class.  For
     * non-iterable objects, `Exc_set` is raised with the Python exception set
     * by the `PyObject_GetIter` function.
     */

    Iter_handle begin() const;

    /** Gets the sentinel iterator testing the end of an iterator.
     *
     * To make the iteration more C++ idiomatic, this method constructs the
     * default one-past-end sentinel iterator of `Iter_handle` type.
     */

    Iter_handle end() const noexcept;

    //
    // Buffer protocol
    //

    //
    // Fundamental objects
    //

    // Type object
    //
    // WIP: These methods are not fully tested yet.

    /** Checks if the handled object is a type object.
     */

    bool check_type() const noexcept { return PyType_Check(get()); }

    /** Gets the pointer to the handled type.
     *
     * Note that assertion will fail if the handle is not holding an actual
     * type object.  It is the responsiblity of the caller to check before
     * calling this method.
     */

    PyTypeObject* as_type() const noexcept
    {
        assert(*this && check_type());
        return (PyTypeObject*)get();
    }

    //
    // Numeric objects
    //

    /** Builds a built-in Python int object.
     */

    Handle(long v)
        : Handle(PyLong_FromLong(v))
    {
    }

    Handle(unsigned long v)
        : Handle(PyLong_FromUnsignedLong(v))
    {
    }

    /** Reads a Python integer into a C++ integral object.
     */

    void as(long& out) const
    {
        out = PyLong_AsLong(ref_);
        check_exc();
    }

    void as(unsigned long& out) const
    {
        out = PyLong_AsUnsignedLong(ref_);
        check_exc();
    }

    //
    // Sequence objects
    //

    /** Checks if the handled object is a tuple instance.
     */

    bool check_tuple() const noexcept { return PyTuple_Check(get()); }

    //
    // Container objects
    //

    //
    // Function objects
    //

    //
    // Other objects
    //

private:
    //
    // Reference counting handling.
    //

    /** Sets the current handle to refer to the given object.
     *
     * Note that this internal method does not take care of the previous values
     * of the fields.
     */

    void set(PyObject* ref, Own own, bool allow_null)
    {
        ref_ = ref;
        if_borrow_ = own == BORROW;

        if (ref_ == nullptr && !allow_null) {
            throw Exc_set();
        }
        if (own == NEW && ref_ != nullptr) {
            Py_INCREF(ref_);
        }
    }

    /** Decrements the reference count for owning reference to a non-null.
     */

    void decr_ref() noexcept
    {
        if (!if_borrow_) {
            Py_XDECREF(ref_);
        }
    }

    /** Increments the reference count only for owning reference.
     */

    void incr_ref() noexcept
    {
        if (!if_borrow_) {
            Py_INCREF(ref_);
        }
    }

    //
    // Python object comparisons
    //

    template <int op> bool compare(const Handle o) const
    {
        int res = PyObject_RichCompareBool(get(), o.get(), op);
        if (res == -1) {
            throw Exc_set();
        }
        return res == 1;
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

//
// Utility non-member handle functions
//

/** Gets a new reference from a handle.
 *
 * When applied on R-values of owning handles, it could save two bumpings of
 * the reference count.
 */

inline PyObject* get_new(const Handle& handle) noexcept
{
    return handle.get_new();
}

inline PyObject* get_new(Handle&& handle) noexcept { return handle.release(); }

//
// Utilities for iterator protocol
//

/** Wrapper over a Python iterator.
 *
 * This class aims to facilitate the iteration over Python iterators.  A Python
 * iterator can be wrapped inside and the Python object can be obtained by the
 * C++ dereferencing operator `*`, with the C++ `++` operator denoting
 * advancement.
 *
 * The end of the iterator can be tested in either the Python/Java way of
 * calling a unary predicate (a method named `has_val` here), or handled in the
 * idiomatic C++ way of binary comparison with the one-past-end sentinel, which
 * can be constructed by using the default constructor.  Especially, it satisfy
 * the C++ `ForwardIterator` concept.
 *
 * Normally, for iterating over Python iterable objects, the `Handle::begin`
 * and `Handle::end` method can be used for the iterable, instead of direct
 * construction of objects of this class, although this class can be used
 * directly on any Python iterators from other sources.
 *
 * Note that different from specialized handles for things like modules or
 * tuples, here the basic handle interface is not exposed.  The behaviour
 * differs a lot from other Python object handles for the sake of better
 * interoperability with C++ looping idioms.  This is also a reason for its
 * different naming.
 */

class Iter_handle : private Handle {
public:
    using value_type = Handle;
    using difference_type = ptrdiff_t;
    using reference = const Handle&;
    using pointer = const Handle*;
    using iterator_category = std::input_iterator_tag;

    /** Constructs an end sentinel.
     *
     * By default, the past-the-end sentinel is constructed.
     */

    Iter_handle() noexcept
        : Handle{}
        , val_{}
    {
    }

    /** Constructs a handle for a Python iterator.
     *
     * Reference will be stolen and a null pointer will be taken to indicate
     * that an exception is already set on Python runtime, for the convenience
     * of working with `PyObject_GetIter` function.
     *
     * If the given object is not an iterator, Python exception will be set and
     * `Exc_set` exception will be thrown.
     */

    Iter_handle(PyObject* obj)
        : Handle(obj)
        , val_{}
    {
        // Adapted from CPython Python/bltinmodule.c for consistency.
        if (!PyIter_Check(get())) {
            PyErr_Format(PyExc_TypeError, "'%.200s' object is not an iterator",
                get()->ob_type->tp_name);
            throw Exc_set();
        }

        set_next();
    }

    /** Makes equality comparison with the sentinel.
     *
     * Note that we can only make equality comparison with a past-the-end
     * sentinel.  Otherwise an assertion will fail.
     */

    friend bool operator!=(
        const Iter_handle& o1, const Iter_handle& o2) noexcept
    {
        // One of them has to be a sentinel.
        assert(!o1 != !o2);

        if (!o1) {
            // When o1 is the sentinel.
            return o2.has_val();
        } else {
            return o1.has_val();
        }
    }

    friend bool operator==(
        const Iter_handle& o1, const Iter_handle& o2) noexcept
    {
        return !(o1 != o2);
    }

    /** Dereferences the iterator.
     *
     * Note that an empty handle will be returned if the iterator is no longer
     * dereferenceable.
     */

    const Handle& operator*() const noexcept { return val_; }

    /** Increments the iterator.
     */

    Iter_handle& operator++()
    {
        set_next();
        return *this;
    }

    /** Tests if the iterator has an value.
     */

    bool has_val() const noexcept { return bool(val_); }

private:
    /** Sets the next object from the Python iterator as the current value.
     *
     * The underlying Python iterator is also incremented in this way.
     */

    void set_next()
    {
        val_.reset(PyIter_Next(get()), STEAL, true);
        if (!val_) {
            if (PyErr_Occurred())
                throw Exc_set();
        }
    }

    /** The current object from the iterator.
     */

    Handle val_;
};

inline Iter_handle Handle::begin() const
{
    return Iter_handle(PyObject_GetIter(ref_));
}

inline Iter_handle Handle::end() const noexcept { return Iter_handle{}; }

//
// Utilities for fundamental objects
//

/** Static types.
 *
 * This type is aimed to facilitate the handling of type objects stored
 * statically.  So it is not a subclass of the main Handle class.
 *
 * Normally some initialization of the static type object cannot happen before
 * the actual starting of the entire Python stack.  So we cannot put all
 * initialization processes inside a constructor and rely on C++ static
 * initialization facility.  So internally, along with the type object, we also
 * have a boolean giving if the type object is ready to be used.  And we have a
 * few methods to make the type ready based on different kinds of information.
 * For all of them, if the type has been made ready before, nothing is
 * performed and a false will be returned.  Otherwise, the initialization by
 * the method is executed and the type starts to be considered to be ready.
 *
 * TODO: This utility is not yet fully designed or developed.
 */

class Static_type {
public:
    /** Constructs from a given type object.
     *
     * Normally the type object template can be just given as a
     * brance-init-list for initializing he CPython type object.  The
     * readiness is set to false by default.
     */

    Static_type(const PyTypeObject& tp)
        : tp_(tp)
    {
    }

    /** Constructs an empty object.
     *
     * The result will be considered to be not ready by default.
     */

    Static_type() {}

    /** Tests if the current static type is ready.
     */

    bool is_ready() const noexcept { return is_ready_; }

    /** Gets the pointer to the underlying type.
     */

    PyTypeObject* tp() noexcept { return &tp_; }

    /** Get the pointer to the underlying type as object pointers.
     */

    PyObject* tp_obj() noexcept { return (PyObject*)tp(); }

    /** Gets a handle for the underlying type object.
     *
     * This can be helpful for working with other cpypp functions expecting a
     * handle.
     */

    Handle get_handle(bool if_new = false)
    {
        return { tp_obj(), if_new ? NEW : BORROW };
    }

    /** Makes the underlying type ready.
     *
     * This is the most general initialization method.  The given callable will
     * be called with a pointer to the type object for the initialization
     * operations.  Then the CPython `PyType_Ready` function will be called.
     *
     * If the type has already been made ready, nothing will be done and a
     * false will be returned.  Otherwise it will be made ready with a true
     * returned.
     */

    template <typename T> bool make_ready(T action)
    {
        if (is_ready_) {
            return false;
        }

        action(&tp_);
        if (PyType_Ready(&tp_) < 0) {
            throw Exc_set{};
        }

        is_ready_ = true;
        return true;
    }

    /** Initializes the static type as a struct sequence.
     *
     * Due to CPython issue28709, struct sequence types cannot be put onto the
     * heap with ease.  By using this method, struct sequence types can be
     * constructed easily on static memory.
     */

    bool make_ready(PyStructSequence_Desc& desc)
    {
        if (is_ready_) {
            return false;
        }

        if (PyStructSequence_InitType2(&tp_, &desc) == -1) {
            throw Exc_set{};
        }

        is_ready_ = true;
        return true;
    }

private:
    /** The actual Python type object.
     */

    PyTypeObject tp_;

    /** If the type object is ready to be used.
     */

    bool is_ready_ = false;
};

//
// Utilities for numeric objects
//

//
// Utilities for sequence objects
//

/** Handles for tuples.
 *
 * This class is mostly designed for creating new tuples rather than reading
 * existing tuples, for which the generic sequence or iterable protocol is
 * better used.
 */

class Tuple : public Handle {
public:
    /** Constructs a tuple of the given length.
     */

    Tuple(Py_ssize_t len)
        : Handle(PyTuple_New(len))
    {
    }

    /** Sets an item for the tuple at the given position.
     */

    template <typename T> void setitem(Py_ssize_t pos, T&& v)
    {
        PyTuple_SET_ITEM(get(), pos, cpypp::get_new(std::forward<T>(v)));
    }
};

/** Handles for CPython struct sequence objects.
 */

class Struct_sequence : public Handle {
public:
    /** Constructs a struct sequence from the given Python type object.
     *
     * Note that it is the responsiblity of the caller to make sure that the
     * given type is indeed a struct sequence type.
     */

    Struct_sequence(PyTypeObject* tp)
        : Handle(PyStructSequence_New(tp))
    {
    }

    /** Constructs an new object from the given static type.
     */

    Struct_sequence(Static_type& tp)
        : Struct_sequence(tp.tp())
    {
    }

    /** Sets the item at a given position.
     */

    template <typename T> void setitem(Py_ssize_t pos, T&& v)
    {
        PyStructSequence_SetItem(
            get(), pos, cpypp::get_new(std::forward<T>(v)));
        return;
    }
};

//
// Utilities for container objects
//

//
// Utilities for function objects
//

//
// Utilities for other objects
//

/** Handles for Python modules.
 *
 * This subclass primarily just adds some utility methods for working with
 * modules.
 */

class Module : public Handle {
public:
    /** Constructs a handle managing the given module.
     *
     * Since normally when working with modules in extentions, especially for
     * the new multi-phase initialization, we only need to work with a borrowed
     * reference to a module.  So different from the corresponding constructor
     * of the base class, this constructor does not steal a reference by
     * default.
     */

    Module(PyObject* mod, Own own = BORROW, bool allow_null = false)
        : Handle(mod, own, allow_null)
    {
    }

    /** Adds an object to the module.
     */

    template <typename T> void add_object(const char* name, T&& v)
    {
        if (PyModule_AddObject(get(), name, cpypp::get_new(std::forward<T>(v)))
            != 0) {
            throw Exc_set{};
        }
        return;
    }
};

// End of namespace cpypp
}

#endif
