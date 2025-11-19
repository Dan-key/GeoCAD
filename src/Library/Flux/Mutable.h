#pragma once

#include <memory>
#include <vector>
#include <functional>

namespace Flux {

template<typename T>
class Mutable
{
public:
    Mutable() : _v(std::make_shared<T>()) {}
    Mutable(const T& value) : _v(std::make_shared<T>(value)) {}
    Mutable(const Mutable<T>& other) : _v(other._v) {}
    Mutable<T>& operator=(const Mutable<T>& other) {
        if (this != &other) {
            _v = other._v;
        }
        return *this;
    }

    T get() const {
        return *_v;
    }

    const T& value() const {
        return *_v;
    }

    void set(const T& value) {
        *_v = value;
        for (const auto& observer : _observers) {
            observer(*_v);
        }
    }

    void subscribe(std::function<void(const T&)> observer) {
        _observers.push_back(observer);
    }
private:
    std::shared_ptr<T> _v;
    std::vector<std::function<void(const T&)>> _observers;
};

} // namespace Flux