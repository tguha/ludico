#pragma once

struct CloneBase { };

// TODO: allow for use with custom allocator, don't depend on 'new'
template <class D, class B>
struct Clone : public B {

    virtual B *_clone() const {
        return new D(*static_cast<const D *>(this));
    }

    D *clone() const {
        return static_cast<D *>(this->_clone());
    }

    using B::B;
};