#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <QVector>
#include <qcontainerfwd.h>
#include <vector>

namespace Flux {

template<typename T>
class MutableList
{
public:
    MutableList()
        : _list(std::make_shared<QVector<T>>()),
          _observers(std::make_shared<std::vector<std::function<void(const T &, size_t)>>>())
    {}

    MutableList(const QVector<T> &list)
        : _list(std::make_shared<QVector<T>>(list)),
          _observers(std::make_shared<std::vector<std::function<void(const T &, size_t)>>>()) 
    {}

    MutableList(const MutableList<T> &other)
        : _list(other._list), _observers(other._observers)
    {}

    MutableList<T> &operator=(const MutableList<T> &other) {
        if (this != &other) {
        _list = other._list;
        _observers = other._observers;
        }
        return *this;
    }

    QVector<T> get() const {
        return *_list;
    }

    const QVector<T>& value() const {
        return *_list;
    }

    const T& at(size_t index) const {
        return (*_list)[index];
    }

    void add(const T& value) {
        _list->append(value);
        size_t index = _list->size() - 1;
        for (const auto& observer : *_observers) {
            observer(value, index);
        }
    }

    void update(size_t index, const T& value) {
        if (index < _list->size()) {
            (*_list)[index] = value;
            for (const auto& observer : *_observers) {
                observer(value, index);
            }
        }
    }

    void set(const QVector<T>& list) {
        *_list = list;
        for (size_t i = 0; i < _list->size(); ++i) {
            for (const auto& observer : *_observers) {
                observer((*_list)[i], i);
            }
        }
    }

    void subscribe(std::function<void(const T&, size_t index)> observer) {
        _observers->push_back(observer);
    }

    size_t size() const {
        return _list->size();
    }

    using value_type = T;
private:
    std::shared_ptr<QVector<T>> _list;
    std::shared_ptr<std::vector<std::function<void(const T&, size_t index)>>> _observers;
};

} //namespace Flux