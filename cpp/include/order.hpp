#pragma once

#include <vector>
#include <string>

namespace execution {

struct Order {
    double size;
    std::string direction;
    int num_slices;

    // Constructor
    Order(double size, std::string dir, int num_slices)
        : size(size), direction(dir), num_slices(num_slices) {};
};

struct ExecutionSlice

} // namespace execution