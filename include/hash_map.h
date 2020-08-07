#pragma once

#include "policy.h"
#include "hash_table.h"
#include <functional>

template<
        class Key,
        class T,
        class CollisionPolicy = LinearProbing,
        class Hash = std::hash<Key>,
        class Equal = std::equal_to<Key>
>
class HashMap {

    template<class It, class V>
    class HashMapIterator;

public:
    // types
    using value_type = std::pair<const Key, T>;
    using hasher = Hash;
    using key_equal = Equal;
    using size_type = std::size_t;
    using Table = HashTable<Key, value_type, CollisionPolicy, Hash, Equal>;
    using key_type = Key;
    using mapped_type = T;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    using iterator = HashMapIterator<typename Table::iterator, value_type>;
    using const_iterator = HashMapIterator<typename Table::const_iterator, const value_type>;

    explicit HashMap(size_type expected_max_size = 4,
                     const hasher &hash = hasher(),
                     const key_equal &equal = key_equal()) : table(
            [](const value_type &val) -> const Key & { return val.first; }, expected_max_size, hash, equal) {}

    template<class InputIt>
    HashMap(InputIt first, InputIt last,
            size_type expected_max_size = 4,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) : table(first, last,
                                                          [](const value_type &val) -> const Key & { return val.first; },
                                                          expected_max_size, hash, equal) {}

    HashMap(const HashMap &) = default;

    HashMap(HashMap &&) noexcept = default;

    HashMap(std::initializer_list<value_type> init,
            size_type expected_max_size = 4,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) : table(init,
                                                          [](const value_type &val) -> const Key & { return val.first; },
                                                          expected_max_size, hash, equal) {}

    HashMap &operator=(const HashMap &other) {
        swap(HashMap(other));
        return *this;
    };

    HashMap &operator=(HashMap &&other) noexcept {
        swap(std::move(other));
        return *this;
    };

    HashMap &operator=(std::initializer_list<value_type> init) {
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
        auto tmp = table.emplace(key);
        return {iterator(tmp.first), tmp.second};
    }

    std::pair<iterator, bool> insert(value_type &&key) {
        auto tmp = table.emplace(std::move(key));
        return {iterator(tmp.first), tmp.second};
    }

    iterator insert(const_iterator hint, const value_type &key) {
        return iterator(table.emplace_hint(hint, key));
    }

    iterator insert(const_iterator hint, value_type &&key) {
        return iterator(table.emplace_hint(hint, std::move(key)));
    }

    template<class P>
    std::pair<iterator, bool> insert(P &&value) {
        return emplace(std::forward<P>(value));
    }

    template<class P>
    std::pair<iterator, bool> insert(const_iterator, P &&value) {
        return emplace(std::forward<P>(value));
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        table.insert(first, last);
    }

    void insert(std::initializer_list<value_type> init) {
        table.insert(init);
    }

    template<class M>
    std::pair<iterator, bool> insert_or_assign(const key_type &key, M &&value) {
        std::pair<iterator, bool> res = try_emplace(key, std::forward<M>(value));
        if (!res.second) {
            res.first->second = std::forward<M>(value);
        }
        return {iterator(res.first), res.second};
    }

    template<class M>
    std::pair<iterator, bool> insert_or_assign(key_type &&key, M &&value) {
        std::pair<iterator, bool> res = try_emplace(std::move(key), std::forward<M>(value));
        if (!res.second) {
            res.first->second = std::forward<M>(value);
        }
        return {iterator(res.first), res.second};
    }

    template<class M>
    iterator insert_or_assign(const_iterator, const key_type &key, M &&value) {
        return insert_or_assign(key, std::forward<M>(value)).first;
    }

    template<class M>
    iterator insert_or_assign(const_iterator, key_type &&key, M &&value) {
        return insert_or_assign(std::move(key), std::forward<M>(value)).first;
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

    template<class... Args>
    std::pair<iterator, bool> try_emplace(const key_type &key, Args &&... args) {
        auto it = find(key);
        if (it != end()) {
            return {it, false};
        }
        return emplace(std::piecewise_construct,
                       std::forward_as_tuple(key),
                       std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template<class... Args>
    std::pair<iterator, bool> try_emplace(key_type &&key, Args &&... args) {
        auto it = find(key);
        if (it != end()) {
            return {it, false};
        }
        return emplace(std::piecewise_construct,
                       std::forward_as_tuple(std::move(key)),
                       std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template<class... Args>
    iterator try_emplace(const_iterator, const key_type &key, Args &&... args) {
        return try_emplace(key, std::forward<Args>(args)...).first;
    }

    template<class... Args>
    iterator try_emplace(const_iterator, key_type &&key, Args &&... args) {
        return try_emplace(std::move(key), std::forward<Args>(args)...).first;
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
    void swap(HashMap &&other) noexcept {
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
        return find(key) != end();
    }

    std::pair<iterator, iterator> equal_range(const key_type &key) {
        auto tmp = table.equal_range(key);
        return {iterator(tmp.first), iterator(tmp.second)};
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const {
        auto tmp = table.equal_range(key);
        return {const_iterator(tmp.first), const_iterator(tmp.second)};
    }

    mapped_type &at(const key_type &key) {
        iterator it = find(key);
        if (it == end()) {
            throw std::out_of_range("No such key");
        }
        return it->second;
    }

    const mapped_type &at(const key_type &key) const {
        const_iterator it = find(key);
        if (it == end()) {
            throw std::out_of_range("No such key");
        }
        return it->second;
    }

    mapped_type &operator[](const key_type &key) {
        return try_emplace(key).first->second;
    }

    mapped_type &operator[](key_type &&key) {
        return try_emplace(std::move(key)).first->second;
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
    friend bool operator==(const HashMap &lhs, const HashMap &rhs) {
        return lhs.table == rhs.table;
    }

    friend bool operator!=(const HashMap &lhs, const HashMap &rhs) {
        return lhs.table != rhs.table;
    }

private:
    template<class It, class V>
    class HashMapIterator {
    private:
        It it;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = V;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        explicit HashMapIterator(It it) : it(it) {}

        HashMapIterator(const HashMapIterator &other) = default;

        It source() {
            return it;
        }

        reference operator*() const {
            return it->data();
        }

        pointer operator->() const {
            return &it->data();
        }

        HashMapIterator &operator++() {
            ++it;
            return *this;
        }

        HashMapIterator operator+(std::size_t shift) {
            return HashMapIterator(it + shift);
        }

        HashMapIterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const HashMapIterator &other) const {
            return other.it == it;
        }

        bool operator!=(const HashMapIterator &other) const {
            return !(*this == other);
        }

        operator HashMapIterator<typename Table::const_iterator, const value_type>() const {
            return const_iterator(it);
        }
    };

    Table table;
};
