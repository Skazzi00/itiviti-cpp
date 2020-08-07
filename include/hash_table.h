#pragma once

#include "policy.h"
#include <functional>
#include <vector>
#include <utility>
#include <typeinfo>
#include <iostream>
#include <memory>

template<
        class Key,
        class Value,
        class CollisionPolicy = LinearProbing,
        class Hash =  std::hash<Value>,
        class Equal = std::equal_to<Value>
>
class HashTable {
private:
    struct Element;

    template<class T>
    class HashTableIterator {
    private:
        T cur_;
        T end_;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type =  typename T::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = typename T::pointer;
        using reference = typename T::reference;

        HashTableIterator(T begin, T end) : cur_(begin), end_(end) {}

        HashTableIterator(const HashTableIterator<T> &other) = default;

        reference operator*() const {
            return *cur_;
        }

        T source() const {
            return cur_;
        }

        pointer operator->() const {
            return &*cur_;
        }

        HashTableIterator &operator++() {
            do {
                cur_++;
            } while (cur_ != end_ && cur_->type != DATA);
            return *this;
        }

        HashTableIterator operator+(std::size_t shift) {
            auto tmp = *this;
            for (std::size_t i = 0; i < shift; ++i) {
                ++tmp;
            }
            return tmp;
        }

        HashTableIterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const HashTableIterator<T> &other) const {
            return other.cur_ == cur_ && other.end_ == end_;
        }

        bool operator!=(const HashTableIterator<T> &other) const {
            return !(*this == other);
        }

        operator HashTableIterator<typename std::vector<Element>::const_iterator>() const {
            return const_iterator(cur_, end_);
        }
    };

public:
    using key_type = Key;
    using value_type = Value;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = Equal;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    using iterator = HashTableIterator<typename std::vector<Element>::iterator>;
    using const_iterator = HashTableIterator<typename std::vector<Element>::const_iterator>;

    explicit HashTable(std::function<const key_type &(const_reference)> getKV,
                       size_type expected_max_size = 1,
                       const hasher &hash = hasher(),
                       const key_equal &equal = key_equal()) : hash_(hash), equal_(equal),
                                                               data_(optimal_size(
                                                                       expected_max_size / max_load_factor_)),
                                                               key_from_value_(getKV) {
    }

    template<class InputIt>
    HashTable(InputIt first, InputIt last,
              std::function<const key_type &(const_reference)> getKV,
              size_type expected_max_size = 1,
              const hasher &hash = hasher(),
              const key_equal &equal = key_equal()) : hash_(hash), equal_(equal),
                                                      data_(optimal_size(expected_max_size / max_load_factor_)),
                                                      key_from_value_(getKV) {
        for (auto i = first; i != last; i++) {
            insert(*i);
        }
    }

    HashTable(const HashTable &o) = default;

    HashTable(HashTable &&o) noexcept = default;

    HashTable(std::initializer_list<value_type> init,
              std::function<const key_type &(const_reference)> getKV,
              size_type expected_max_size = 1,
              const hasher &hash = hasher(),
              const key_equal &equal = key_equal()) : hash_(hash), equal_(equal),
                                                      data_(expected_max_size / max_load_factor_), key_from_value_(getKV) {
        insert(init);
    }

    HashTable &operator=(const HashTable &other) {
        HashTable tmp(other);
        swap(tmp);
        return *this;
    }

    HashTable &operator=(HashTable &&other) noexcept {
        swap(std::move(other));
        return *this;
    };

    HashTable &operator=(std::initializer_list<value_type> init) {
        clear();
        insert(init);
    }

    iterator begin() noexcept {
        return iterator(find_begin(), data_.end());
    }

    const_iterator begin() const noexcept {
        return const_iterator(find_begin(), data_.end());
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(find_begin(), data_.end());
    }

    iterator end() noexcept {
        return iterator(data_.end(), data_.end());
    }

    const_iterator end() const noexcept {
        return const_iterator(data_.end(), data_.end());
    }

    const_iterator cend() const noexcept {
        return const_iterator(data_.end(), data_.end());
    }

    bool empty() const {
        return size_ == 0;
    }

    size_type size() const {
        return size_;
    }

    size_type max_size() const {
        return data_.max_size() * max_load_factor_;
    }

    void clear() {
        data_.clear();
        data_.resize(8);
        size_ = 0;
        cells_cnt_ = 0;
    };


    std::pair<iterator, bool> insert(const value_type &value) {
        return emplace(value);
    }

    std::pair<iterator, bool> insert(value_type &&value) {
        return emplace(std::move(value));
    }


    iterator insert(const_iterator, const value_type &value) {
        return emplace(value).first;
    }

    iterator insert(const_iterator, value_type &&value) {
        return emplace(std::move(value)).first;
    }


    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto i = first; i != last; ++i) {
            emplace(*i);
        }
    }

    void insert(std::initializer_list<value_type> init) {
        for (const auto &i : init) {
            emplace(i);
        }
    }


    // construct element in-place, no copy or move operations are performed;
    // element's constructor is called with exact same arguments as `emplace` method
    // (using `std::forward<Args>(args)...`)
    template<class... Args>
    std::pair<iterator, bool> emplace(Args &&... args) {
        check_size();
        Element tmp = Element(std::make_unique<value_type>(std::forward<Args>(args)...));
        size_type hash = hash_(key_from_value_(tmp.data())) % data_.size();
        auto it = CollisionPolicy(data_.size(), hash);
        for (; data_[*it].type != FREE; ++it) {
            if (data_[*it].type == DATA && equal_(key_from_value_(data_[*it].data()), key_from_value_(tmp.data()))) {
                return {iterator(data_.begin() + *it, data_.end()), false};
            }
        }
        data_[*it] = std::move(tmp);
        ++size_;
        ++cells_cnt_;
        return {iterator(data_.begin() + *it, data_.end()), true};
    }

    template<class... Args>
    iterator emplace_hint(const_iterator, Args &&... args) {
        return emplace(std::forward<Args>(args)...).first;
    }

    iterator erase(const_iterator pos) {
        size_type id = pos.source() - data_.begin();
        auto res = iterator(data_.begin() + id, data_.end()) + 1;
        data_[id].free();
        size_--;
        return res;
    }

    iterator erase(const_iterator first, const_iterator last) {
        size_type id = last.source() - data_.begin();
        auto res = iterator(data_.begin() + id, data_.end()) + 1;
        for (auto i = first; i != last; ++i) {
            erase(i);
        }
        return res;
    }

    iterator find(const key_type &key) {
        auto hash = hash_(key);
        auto it = CollisionPolicy(data_.size(), hash);
        for (; data_[*it].type != FREE; ++it) {
            if (data_[*it].type == DATA && equal_(key_from_value_(data_[*it].data()), key)) {
                return iterator(data_.begin() + *it, data_.end());
            }
        }
        return end();
    }

    const_iterator find(const key_type &key) const {
        auto hash = hash_(key);
        auto it = CollisionPolicy(data_.size(), hash);
        for (; data_[*it].type != FREE; ++it) {
            if (data_[*it].type == DATA && equal_(key_from_value_(data_[*it].data()), key)) {
                return const_iterator(data_.begin() + *it, data_.end());
            }
        }
        return cend();
    }


    size_type erase(const key_type &key) {
        auto it = find(key);
        size_type cnt = 0;
        do {
            erase(it);
            it = find(key);
            cnt++;
        } while (it != end());
        return cnt;
    }

    // exchanges the contents of the container with those of other;
    // does not invoke any move, copy, or swap operations on individual elements
    void swap(HashTable &&other) noexcept {
        std::swap(other.data_, data_);
        std::swap(other.equal_, equal_);
        std::swap(other.hash_, hash_);
        std::swap(other.size_, size_);
        std::swap(other.cells_cnt_, cells_cnt_);
        std::swap(other.key_from_value_, key_from_value_);
    }

    size_type count(const key_type &key) const {
        return find(key) != end() ? 1 : 0;
    }


    bool contains(const key_type &key) const {
        return count(key) == 1;
    }

    std::pair<iterator, iterator> equal_range(const key_type &key) {
        auto tmp = find(key);
        return tmp == end() ? make_pair(end(), end()) : make_pair(tmp, tmp + 1);
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const {
        auto tmp = find(key);
        return tmp == cend() ? make_pair(end(), end()) : make_pair(tmp, tmp + 1);
    };


    size_type bucket_count() const {
        return data_.size();
    }

    size_type max_bucket_count() const {
        return data_.max_size();
    }

    size_type bucket_size(const size_type) const {
        return 1;
    }

    size_type bucket(const key_type &key) const {
        return std::distance(data_.begin(), find(key).source());
    }

    float load_factor() const {
        return size_ * 1. / data_.size();
    }

    float max_load_factor() const {
        return max_load_factor_;
    }

    void rehash(size_type count) {
        if (count == bucket_count()) return;
        if (count < size() / max_load_factor()) {
            count = size() / max_load_factor();
        }
        std::vector<Element> old(count);
        std::swap(old, data_);
        size_ = 0;
        cells_cnt_ = 0;
        for (auto &el: old) {
            if (el.type == DATA) {
                insert(el);
            }
        }
    }

    void reserve(size_type count) {
        rehash(optimal_size(count / max_load_factor()));
    }

    // compare two containers contents
    friend bool operator==(const HashTable &lhs, const HashTable &rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (auto const &el: lhs) {
            if (!rhs.contains(lhs.key_from_value_(el.data()))) {
                return false;
            }
        }
        return true;
    }

    friend bool operator!=(const HashTable &lhs, const HashTable &rhs) {
        return !(lhs == rhs);
    }

private:
    typename std::vector<Element>::iterator find_begin() {
        auto it = data_.begin();
        while (it != data_.end()) {
            if (it->type == DATA)
                return it;
            it++;
        }
        return it;
    }

    typename std::vector<Element>::const_iterator find_begin() const {
        auto it = data_.begin();
        while (it != data_.end()) {
            if (it->type == DATA)
                return it;
            it++;
        }
        return it;
    }

    enum ElementType {
        FREE, DELETED, DATA
    };

    struct Element {
        using Type = value_type;
        ElementType type;
        std::unique_ptr<Type> ptr;

        Element() : type(FREE) {}

        Element(std::unique_ptr<Type> data) : type(DATA), ptr(std::move(data)) {}

        Element(const Element &element) : type(element.type) {
            if (element.ptr != nullptr) {
                ptr = std::make_unique<Type>(element.data());
            }
        };

        Element(Element &&element) noexcept {
            swap(std::move(element));
        }

        Element &operator=(const Element &other) {
            Element tmp(other);
            swap(tmp);
            return *this;
        }

        Element &operator=(Element &&other) noexcept {
            swap(std::move(other));
            return *this;
        }

        void swap(Element &&element) noexcept {
            std::swap(element.type, type);
            std::swap(element.ptr, ptr);
        }

        const Type &data() const {
            return *ptr;
        }

        Type &data() {
            return *ptr;
        }

        template<class... Args>
        void set(Args &&... args) {
            type = DATA;
            ptr = std::make_unique<Type>(std::forward<Args>(args)...);
        }


        void free() {
            type = DELETED;
            ptr.reset(nullptr);
        }
    };

    static size_type optimal_size(size_type sz) {
        size_type k = 1ull << (sizeof(size_type) * 8 - 1);
        while ((sz & k) == 0) {
            k >>= 1u;
        }
        return k << 1u;
    }

    void check_size() {
        if (cells_cnt_ > max_load_factor() * bucket_count()) {
            rehash(bucket_count() / max_load_factor());
        }
    }

    void insert(Element &element) {
        check_size();
        size_type hash = hash_(key_from_value_(element.data())) % data_.size();
        auto it = CollisionPolicy(data_.size(), hash);
        for (; data_[*it].type != FREE; ++it) {
            if (data_[*it].type == DATA && equal_(key_from_value_(data_[*it].data()), key_from_value_(element.data()))) {
                return;
            }
        }
        data_[*it] = std::move(element);
        ++size_;
        ++cells_cnt_;
    }

    hasher hash_;
    key_equal equal_;
    std::vector<Element> data_;
    size_type size_ = 0;
    size_type cells_cnt_ = 0;
    std::function<const key_type &(const_reference)> key_from_value_;
    constexpr static float max_load_factor_ = 0.5;
};