#pragma once

#include "array_ptr.h"
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <type_traits>
#include <utility>

struct ReserveProxyObj {            //proxy structure for SimpleVector(Reserve())
    explicit ReserveProxyObj(size_t capacity_to_reserve) : capacity(capacity_to_reserve) {}
    size_t capacity;
};

inline ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) {
        Initialize(size);
        std::fill(begin(), end(), Type());
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        Initialize(size);
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        Initialize(init.size());
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(ReserveProxyObj reserved)
        : ptr_data_(reserved.capacity)
        , size_(0)
        , capacity_(reserved.capacity) {
    }

    SimpleVector(const SimpleVector& other) {
        Initialize(other.size_);
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector& operator = (const SimpleVector& rhs) {
        if (ptr_data_.Get() != rhs.ptr_data_.Get()) {
            auto temp(rhs);
            swap(temp);
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other) {
        swap(other);
    }

    SimpleVector& operator = (SimpleVector&& rhs) {
        if (ptr_data_.Get() != rhs.ptr_data_.Get()) {
            swap(rhs);
        }
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        if (size_ != 0) {
            return false;
        }
        return true;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return ptr_data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return ptr_data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return ptr_data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return ptr_data_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        // Напишите тело самостоятельно
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_) {
            size_ = new_size;
        } else if (new_size <= capacity_) {
            ResizeAndFill(new_size, new_size);
        } else {
            auto new_capacity = std::max(new_size, capacity_ * 2);
            ResizeAndFill(new_size, new_capacity);
        }
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ResizeAndFill(size_, new_capacity);
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return ptr_data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return ptr_data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ptr_data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ptr_data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return ptr_data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ptr_data_.Get() + size_;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ == 0 && capacity_ == 0) {
            Initialize(1);
            ptr_data_[0] = item;
        } else {
            if (size_ == capacity_) {
                ResizeAndFill(size_, 2 * capacity_, true);
            }
            ptr_data_[size_++] = item;
        }
    }

    void PushBack(Type&& item) {
        if (size_ == 0 && capacity_ == 0) {
            Initialize(1);
            ptr_data_[0] = std::move(item);
        } else {
            if (size_ == capacity_) {
                ResizeAndFill(size_, 2 * capacity_, true);
            }
            ptr_data_[size_++] = std::move(item);
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());         //checks if iterator is valid
        assert(ptr_data_[pos - begin()] == *pos);       //checks if pos is from current vector
        if (size_ == 0 || pos == end()) {
            PushBack(value);
            return &ptr_data_[size_ - 1];
        }

        size_t count_before = pos - begin();            //index of pos
        size_t count_after = end() - pos;               //number of elements after pos
        auto temp = SimpleVector(count_after);          //temporary vector for values from [pos] to end()
        std::copy(begin() + count_before, end(), temp.begin());     //copies values from this[pos-to-end()) to temporary vector

        if (size_ == capacity_) {
            ResizeAndFill(size_, capacity_ * 2);
        }
        ptr_data_[count_before] = value;                //saves Insert value to this
        size_++;
        std::copy(temp.begin(), temp.end(), begin() + count_before + 1);        //copies values from temporary vector to this[pos+1, end())

        return &ptr_data_[count_before];
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());         //checks if iterator is valid
        if (size_ == 0 || pos == end()) {
            PushBack(std::move(value));
            return &ptr_data_[size_ - 1];
        }

        size_t count_before = pos - begin();            //index of pos
        size_t count_after = end() - pos;               //number of elements after pos
        auto temp = SimpleVector();          //temporary vector for values from [pos] to end()
        temp.ResizeAndFill(count_after, count_after);
        std::move(begin() + count_before, end(), temp.begin());     //moves values from this[pos-to-end()) to temporary vector

        if (size_ == capacity_) {
            ResizeAndFill(size_, capacity_ * 2);
        }
        ptr_data_[count_before] = std::move(value);                //saves Insert value to this
        size_++;
        std::move(temp.begin(), temp.end(), begin() + count_before + 1);        //moves values from temporary vector to this[pos+1, end())

        return &ptr_data_[count_before];
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(size_ > 0);
            size_--;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());          //checks if iterator is valid
        assert(size_ > 0);                              //checks if iterator is applicable
        size_t index = pos - begin();
        std::move(begin() + index + 1, end(), begin() + index);
        size_--;
        return &ptr_data_[index];
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        ptr_data_.swap(other.ptr_data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

private:
    ArrayPtr<Type> ptr_data_;
    size_t size_ = 0;
    size_t capacity_ = 0;
    // const bool type_copiable = Type(Type());

    void Initialize(size_t size) {        //initialises private fields, creates and swaps ArrayPtr<Type>(size) with [this]
        size_ = size;
        capacity_ = size;
        auto temp = ArrayPtr<Type>(size);
        ptr_data_.swap(temp);
    }

    void SizeCapacityCheck (size_t new_size, size_t new_capacity) {
        if (size_ != new_size) {
            size_ = new_size;
        }
        if (capacity_ != new_capacity && new_capacity >= new_size) {
            capacity_ = new_capacity;
        }
    }

    //RAII procedure for Resize() and PushBack() methods
    //creates ArrayPtr<Type>(new_size), copies existing values and fills with Type() before new_size, then swaps with [this];
    //updates size_ and capacity_ fields with new values; if push_back = true, adds default value Type() in the end()
    void ResizeAndFill(size_t new_size, size_t new_capacity, bool push_back = false) {
        auto temp = ArrayPtr<Type>(new_capacity);
        std::move(ptr_data_.begin(), ptr_data_.begin()+size_, temp.begin());
        for (size_t j = size_; j < new_size; j++) {
            temp[j] = std::move(Type());
        }
        SizeCapacityCheck(new_size, new_capacity);
        if (push_back) {
            temp[size_] = std::move(Type());
        }
        ptr_data_.swap(temp);
    }
};

template <typename Type>
inline bool operator == (const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (std::equal(lhs.begin(), lhs.end(), rhs.begin()) && lhs.GetSize() == rhs.GetSize());
}

template <typename Type>
inline bool operator == (const SimpleVector<Type>&& lhs, const SimpleVector<Type>&& rhs) {
    return (std::equal(lhs.begin(), lhs.end(), rhs.begin()) && lhs.GetSize() == rhs.GetSize());
}

template <typename Type>
inline bool operator != (const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator != (const SimpleVector<Type>&& lhs, const SimpleVector<Type>&& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator < (const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator <= (const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator > (const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator >= (const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
