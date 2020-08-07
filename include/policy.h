#pragma once

#include <cstdlib>

struct LinearProbing {
    std::size_t size;
    std::size_t currentIndex;
    std::size_t start;

    LinearProbing(size_t size, size_t start) : size(size), currentIndex(0), start(start) {}

    std::size_t operator*() {
        return (start + currentIndex) % size;
    }

    LinearProbing operator++() {
        ++currentIndex;
        return *this;
    }
};
struct QuadraticProbing {
    std::size_t size;
    std::size_t currentBase;
    std::size_t start;

    QuadraticProbing(size_t size, size_t start) : size(size), currentBase(0), start(start) {}

    std::size_t operator*() {
        return (start + currentBase * currentBase) % size;
    }

    QuadraticProbing operator++() {
        ++currentBase;
        return *this;
    }
};
