#pragma once

template <typename T>
struct Meta {
    T* value;
    Meta(T* p) : value(p) {}
    T& operator*() const { return *value; }
    Meta& operator=(const T& v) { *value = v; return *this; }
    using NativeType = T;

};

#define EXPOSE(MemberName) \
    Meta<decltype(MemberName)> Meta##MemberName{&MemberName}; \
    decltype(MemberName) get##MemberName() { return *Meta##MemberName; } \
    void set##MemberName(const decltype(MemberName)& v) { Meta##MemberName = v; } \
    using _meta_native_type_##MemberName = typename Meta<decltype(MemberName)>::NativeType; \
    Q_PROPERTY(_meta_native_type_##MemberName MemberName READ get##MemberName WRITE set##MemberName)

