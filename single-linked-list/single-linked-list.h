#pragma once

#include <cassert>
#include <cstddef>
#include <utility>
#include <iterator> // Содержит объявления категорий итераторов

template <typename Type>
class SingleLinkedList {
    // Узел списка
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next)
            : value(val)
            , next_node(next) {}
        Type value;
        Node* next_node = nullptr;
    };

    // Шаблон класса «Базовый Итератор».
    // Определяет поведение итератора на элементы односвязного списка
    // ValueType — совпадает с Type (для Iterator) либо с const Type (для ConstIterator)
    template <typename ValueType>
    class BasicIterator {
        // Класс списка объявляется дружественным, чтобы из методов списка
        // был доступ к приватной области итератора
        friend class SingleLinkedList;

        // Конвертирующий конструктор итератора из указателя на узел списка
        explicit BasicIterator(Node* node) : node_(node) {}

    public:
        // Объявленные ниже типы сообщают стандартной библиотеке о свойствах этого итератора

        // Категория итератора — forward iterator
        // (итератор, который поддерживает операции инкремента и многократное разыменование)
        using iterator_category = std::forward_iterator_tag;
        // Тип элементов, по которым перемещается итератор
        using value_type = Type;
        // Тип, используемый для хранения смещения между итераторами
        using difference_type = std::ptrdiff_t;
        // Тип указателя на итерируемое значение
        using pointer = ValueType*;
        // Тип ссылки на итерируемое значение
        using reference = ValueType&;

        BasicIterator() = default;

        // Конвертирующий конструктор/конструктор копирования
        // При ValueType, совпадающем с Type, играет роль копирующего конструктора
        // При ValueType, совпадающем с const Type, играет роль конвертирующего конструктора
        BasicIterator(const BasicIterator<Type>& other) noexcept : node_(other.node_) {}

        // Чтобы компилятор не выдавал предупреждение об отсутствии оператора = при наличии
        // пользовательского конструктора копирования, явно объявим оператор = и
        // попросим компилятор сгенерировать его за нас
        BasicIterator& operator=(const BasicIterator& rhs) = default;

        // Оператор сравнения итераторов (в роли второго аргумента выступает константный итератор)
        // Два итератора равны, если они ссылаются на один и тот же элемент списка либо на end()
        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }

        // Оператор проверки итераторов на неравенство
        // Противоположен !=
        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return node_ != rhs.node_;
        }

        // Оператор сравнения итераторов (в роли второго аргумента итератор)
        // Два итератора равны, если они ссылаются на один и тот же элемент списка либо на end()
        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }

        // Оператор проверки итераторов на неравенство
        // Противоположен !=
        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return node_ != rhs.node_;
        }

        // Оператор прединкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает ссылку на самого себя
        // Инкремент итератора, не указывающего на существующий элемент списка, приводит к неопределённому поведению
        BasicIterator& operator++() noexcept {
            assert(node_ != nullptr);
            node_ = node_->next_node;
            return *this;
        }

        // Оператор постинкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает прежнее значение итератора
        // Инкремент итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        BasicIterator operator++(int) noexcept {
            auto old = *this;
            ++(*this); // используем логику префиксной формы инкремента
            return old;
        }

        // Операция разыменования. Возвращает ссылку на текущий элемент
        // Вызов этого оператора у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] reference operator*() const noexcept {
            assert(node_ != nullptr);
            Type& ref = node_->value;
            return ref;
        }

        // Операция доступа к члену класса. Возвращает указатель на текущий элемент списка
        // Вызов этого оператора у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] pointer operator->() const noexcept {
            assert(node_ != nullptr);
            return &node_->value;
        }

    private:
        Node* node_ = nullptr;
    };

public:
    SingleLinkedList() = default;

    SingleLinkedList(std::initializer_list<Type> values) {
        for (auto item : values) {
            PushBack(item);
        }
    }

    SingleLinkedList(const SingleLinkedList& other) {
        SingleLinkedList temp;
        for (auto& item : other) {
            temp.PushBack(item);
        }
        swap(temp);
    }

    ~SingleLinkedList() {
        Clear();
    }

    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;

    // Итератор, допускающий изменение элементов списка
    using Iterator = BasicIterator<Type>;
    // Константный итератор, предоставляющий доступ для чтения к элементам списка
    using ConstIterator = BasicIterator<const Type>;

    // Возвращает итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен end()
    [[nodiscard]] Iterator begin() noexcept {
        return Iterator(head_.next_node);
    }

    // Возвращает итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя — попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] Iterator end() noexcept {
        return Iterator(nullptr);
    }

    // Возвращает константный итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен end()
    // Результат вызова эквивалентен вызову метода cbegin()
    [[nodiscard]] ConstIterator begin() const noexcept {
        return ConstIterator(head_.next_node);
    }

    // Возвращает константный итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя — попытка разыменования приведёт к неопределённому поведению
    // Результат вызова эквивалентен вызову метода cend()
    [[nodiscard]] ConstIterator end() const noexcept {
        return ConstIterator(nullptr);
    }

    // Возвращает константный итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен cend()
    [[nodiscard]] ConstIterator cbegin() const noexcept {
        return ConstIterator(head_.next_node);
    }

    // Возвращает константный итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя — попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator cend() const noexcept {
        return ConstIterator(nullptr);
    }

    // Возвращает итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] Iterator before_begin() noexcept {
        return Iterator(&head_);
    }

    // Возвращает константный итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return before_begin();
    }

    // Возвращает константный итератор, указывающий на позицию перед первым элементом односвязного списка.
    // Разыменовывать этот итератор нельзя - попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator before_begin() const noexcept {
        return ConstIterator(const_cast<Node*>(&head_));
    }

    SingleLinkedList& operator = (const SingleLinkedList& rhs) {
        if (this != &rhs) {
            SingleLinkedList temp(rhs);
            Clear();
            swap(temp);
        }
        return *this;
    }

    void swap(SingleLinkedList& other) noexcept {           //swaps two Lists
        std::swap(size_, other.size_);
        auto this_head = head_.next_node;
        head_.next_node = other.head_.next_node;
        other.head_.next_node = this_head;
    }

    // Возвращает количество элементов в списке
    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    // Сообщает, пустой ли список
    [[nodiscard]] bool IsEmpty() const noexcept {
        return (size_ == 0);
    }

    void PushFront(const Type& value) {             //adds element at List begin
        head_.next_node = new Node(value, head_.next_node);
        size_++;
    }

    void PushBack(const Type& value) {              //adds element at List end
        if (size_ == 0) {
            head_.next_node = new Node(value, nullptr);
        } else {
            GetPosition(size_).node_->next_node = new Node(value, nullptr);
        }
        size_++;
    }

    // Вставляет элемент value после элемента, на который указывает pos.
    // Возвращает итератор на вставленный элемент
    // Если при создании элемента будет выброшено исключение, список останется в прежнем состоянии
    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        assert(pos != end());
        auto next_pos_before = pos.node_->next_node;
        pos.node_->next_node = new Node(value, next_pos_before);
        size_++;
        return Iterator(pos.node_->next_node);
    }

    void PopFront() noexcept {          //deletes first element
        assert(size_ != 0);
        auto node_to_delete = head_.next_node;
        head_.next_node = node_to_delete->next_node;
        delete node_to_delete;
    }

    // Удаляет элемент, следующий за pos.
    // Возвращает итератор на элемент, следующий за удалённым
    Iterator EraseAfter(ConstIterator pos) noexcept {
        assert(pos != end());
        assert(pos.node_->next_node != nullptr);
        auto node_to_delete = pos.node_->next_node;
        pos.node_->next_node = node_to_delete->next_node;
        delete node_to_delete;
        size_--;
        return Iterator(pos.node_->next_node);
    }

    void Clear() {
        while (head_.next_node != nullptr) {
            auto next = head_.next_node->next_node;
            delete head_.next_node;
            head_.next_node = next;
        }
        size_ = 0;
    }

private:
    // Фиктивный узел, используется для вставки "перед первым элементом"
    Node head_ = Node();
    size_t size_ = 0;

    Iterator GetPosition(size_t pos) {              //returns Iterator for pos
        assert(pos <= size_);
        size_t i = 1;
        auto before_pos = head_.next_node;
        while (i < pos) {
            before_pos = before_pos->next_node;
            i++;
        }
        return Iterator(before_pos);
    }
};

template <typename Type>
inline void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
inline bool operator == (const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator != (const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator < (const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator <= (const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator > (const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator >= (const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs < rhs);
}
