#pragma once


#include <utility>
#include <vector>
#include <random>
#include <utility>
#include <algorithm>

template<class T, typename Container = std::vector<T>>
class randomized_queue {
    template<class vT, class DataIt>
    struct random_iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = vT;
        using difference_type = std::ptrdiff_t;
        using pointer = vT *;
        using reference = vT &;

        random_iterator(DataIt it, std::mt19937 &rnd, size_t size) : order(size), it(it), cur_pos(0) {
            for (size_t i = 0; i < order.size(); i++) {
                order[i] = i;
            }
            std::shuffle(order.begin(), order.end(), rnd);
        }

        random_iterator(const random_iterator &other) = default;

        random_iterator(std::vector<size_t> order, DataIt &it,
                        size_t curPos) : order(std::move(order)), it(it), cur_pos(curPos) {}


        reference operator*() const {
            return it[order[cur_pos]];
        }

        pointer operator->() const {
            return &it[order[cur_pos]];
        }

        random_iterator &operator++() {
            ++cur_pos;
            return *this;
        }



        random_iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const random_iterator &other) const {
            return it == other.it && cur_pos == other.cur_pos && (cur_pos == order.size() || order == other.order);
        }

        bool operator!=(const random_iterator &other) const {
            return !(*this == other);
        }

        operator random_iterator<const vT, typename Container::const_iterator>() const {
            return const_iterator(order, it, cur_pos);
        };

    private:
        friend randomized_queue<T, Container>;
        random_iterator operator+(int shift) {
            return random_iterator(order, it, cur_pos + shift);
        }
        std::vector<size_t> order;
        DataIt it;
        size_t cur_pos;
    };

public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = random_iterator<T, typename Container::iterator>;
    using const_iterator = random_iterator<const T, typename Container::const_iterator>;

    randomized_queue() = default;

    bool empty() const {
        return data.empty();
    }

    size_t size() const {
        return data.size();
    }

    void enqueue(T &item) {
        data.emplace_back(item);
    }

    void enqueue(T &&item) {
        data.emplace_back(std::move(item));
    }

    T const &sample() const {
        return data[rnd() % data.size()];
    }

    T dequeue() {
        std::swap(data[rnd() % data.size()], data[data.size() - 1]);
        auto res = std::move(data.back());
        data.pop_back();
        return res;
    }

    iterator begin() {
        return iterator(data.begin(), rnd, data.size());
    }

    const_iterator begin() const {
        return const_iterator(data.begin(), rnd, data.size());
    }

    iterator end() {
        return iterator(data.begin(), rnd, data.size()) + data.size();
    }

    const_iterator end() const {
        return const_iterator(data.begin(), rnd, data.size()) + data.size();
    }

    const_iterator cbegin() const {
        return const_iterator(data.cbegin(), rnd, data.size());
    }

    const_iterator cend() const {
        return const_iterator(data.begin(), rnd, data.size()) + data.size();
    }

private:
    Container data;
    mutable std::mt19937 rnd = std::mt19937(std::random_device()());
};
