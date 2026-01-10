#pragma once

#include <vector>
#include <string>

namespace execution {

struct Order {
    double size;
    std::string direction; // buy or sell
    int num_slices;

    // Constructor
    Order(double s, std::string dir, int num_slices)
        : size(s), direction(dir), num_slices(num_slices) {}
};

struct ExecutionSlice {
    int day;
    double size;
    double price;
    double cost;

    // Constructor
    ExecutionSlice(int d, double s, double p, double c)
        : day(d), size(s), price(p), cost(s * p) {}
};

struct ExecutionResult {
    std::vector<ExecutionSlice> slices;
    double total_cost;
    double avg_price;
    double benchmark_price;
    double slippage_bps;

    // Constructor
    ExecutionResult()
        : total_cost(0.0), avg_price(0.0), benchmark_price(0.0), slippage_bps(0.0) {}
};

} // namespace execution