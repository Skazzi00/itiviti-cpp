#pragma once

#include <functional>
#include <iostream>
#include "policy.h"
#include "hash_table.h"

template<
        class Key,
        class CollisionPolicy = LinearProbing,
        class Hash = std::hash<Key>,
        class Equal = std::equal_to<Key>
>
class HashSet {
private:
    using Table = HashTable<Key, Key, CollisionPolicy, Hash, Equal>;

    template<class It>
    class HashSetIterator {
    private:
        It it;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const typename It::value_type::Type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        explicit HashSetIterator(It it) : it(it) {}

        HashSetIterator(const HashSetIterator &other) = default;

        It source() {
            return it;
        }

        reference operator*() const {
            return it->data();
        }

        pointer operator->() const {
            return it->data_;
        }

        HashSetIterator &operator++() {
            ++it;
            return *this;
        }

        HashSetIterator operator+(std::size_t shift) {
            return HashSetIterator(it + shift);
        }

        HashSetIterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const HashSetIterator &other) const {
            return other.it == it;
        }

        bool operator!=(const HashSetIterator &other) const {
            return !(*this == other);
        }

        operator HashSetIterator<typename Table::const_iterator>() const {
            return iterator(it);
        }
    };

    Table table;
public:
    // types
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = Equal;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    using iterator = HashSetIterator<typename Table::const_iterator>;
    using const_iterator =  HashSetIterator<typename Table::const_iterator>;

    explicit HashSet(size_type expected_max_size = 1,
                     const hasher &hash = hasher(),
                     const key_equal &equal = key_equal()) : table(
            [](const_reference val) -> const_reference { return val; }, expected_max_size, hash, equal) {}

    template<class InputIt>
    HashSet(InputIt first, InputIt last,
            size_type expected_max_size = 1,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) : table(first, last,
                                                          [](const_reference val) -> const_reference { return val; },
                                                          expected_max_size, hash, equal) {}

    HashSet(const HashSet &o) : table(o.table) {}

    HashSet(HashSet &&o) noexcept : table(std::move(o.table)) {}

    HashSet(std::initializer_list<value_type> init,
            size_type expected_max_size = 1,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) : table(init,
                                                          [](const_reference val) -> const_reference { return val; },
                                                          expected_max_size, hash, equal) {}

    HashSet &operator=(const HashSet &other) {
        swap(HashSet(other));
        return *this;
    };

    HashSet &operator=(HashSet &&other) noexcept {
        swap(std::move(other));
        return *this;
    };

    HashSet &operator=(std::initializer_list<value_type> init) {
        table = init;
        return *this;
    };

    iterator begin() noexcept {
        return iterator(table.begin());
    }

    const_iterator begin() const noexcept {
        return const_iterator(table.cbegin());
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(table.cbegin());
    }

    iterator end() noexcept {
        return iterator(table.end());
    }

    const_iterator end() const noexcept {
        return const_iterator(table.cend());
    }

    const_iterator cend() const noexcept {
        return const_iterator(table.cend());
    }

    bool empty() const {
        return table.empty();
    }

    size_type size() const {
        return table.size();
    }

    size_type max_size() const {
        return table.max_size();
    }

    void clear() {
        return table.clear();
    }

    std::pair<iterator, bool> insert(const value_type &key) {
        auto tmp = table.insert(key);
        return {iterator(tmp.first), tmp.second};
    }

    std::pair<iterator, bool> insert(value_type &&key) {
        auto tmp = table.insert(std::move(key));
        return {iterator(tmp.first), tmp.second};
    }

    iterator insert(const_iterator hint, const value_type &key) {
        return iterator(table.insert(hint, key));
    }

    iterator insert(const_iterator hint, value_type &&key) {
        return iterator(table.insert(hint, std::move(key)));
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        table.insert(first, last);
    }

    void insert(std::initializer_list<value_type> init) {
        table.insert(init);
    }

    // construct element in-place, no copy or move operations are performed;
    // element's constructor is called with exact same arguments as `emplace` method
    // (using `std::forward<Args>(args)...`)
    template<class... Args>
    std::pair<iterator, bool> emplace(Args &&... args) {
        auto tmp = table.emplace(std::forward<Args>(args)...);
        return {iterator(tmp.first), tmp.second};
    }

    template<class... Args>
    iterator emplace_hint(const_iterator hint, Args &&... args) {
        return iterator(table.emplace_hint(hint.source(), std::forward<Args>(args)...));
    }

    iterator erase(const_iterator pos) {
        return iterator(table.erase(pos.source()));
    }

    iterator erase(const_iterator first, const_iterator last) {
        return iterator(table.erase(first.source(), last.source()));
    }

    size_type erase(const key_type &key) {
        return table.erase(key);
    }

    // exchanges the contents of the container with those of other;
    // does not invoke any move, copy, or swap operations on individual elements
    void swap(HashSet &&other) noexcept {
        std::swap(other.table, table);
    }

    size_type count(const key_type &key) const {
        return table.count(key);
    }

    iterator find(const key_type &key) {
        return iterator(table.find(key));
    }

    const_iterator find(const key_type &key) const {
        return const_iterator(table.find(key));
    }

    bool contains(const key_type &key) const {
        return table.contains(key);
    }

    std::pair<iterator, iterator> equal_range(const key_type &key) {
        auto tmp = table.equal_range(key);
        return {iterator(tmp.first), iterator(tmp.second)};
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const {
        auto tmp = table.equal_range(key);
        return {const_iterator(tmp.first), const_iterator(tmp.second)};
    }

    size_type bucket_count() const {
        return table.bucket_count();
    }

    size_type max_bucket_count() const {
        return table.max_bucket_count();
    }

    size_type bucket_size(const size_type key) const {
        return table.bucket_size(key);
    }

    size_type bucket(const key_type &key) const {
        return table.bucket(key);
    }

    float load_factor() const {
        return table.load_factor();
    }

    float max_load_factor() const {
        return table.max_load_factor();
    }

    void rehash(const size_type count) {
        table.rehash(count);
    }

    void reserve(size_type count) {
        table.reserve(count);
    }

    // compare two containers contents
    friend bool operator==(const HashSet &lhs, const HashSet &rhs) {
        return lhs.table == rhs.table;
    }

    friend bool operator!=(const HashSet &lhs, const HashSet &rhs) {
        return lhs.table != rhs.table;
    }
};
