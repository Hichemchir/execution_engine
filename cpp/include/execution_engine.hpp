#pragma once

#include <order.hpp>
#include <vector>

namespace execution {

class ExecutionEngine {
private:
    double calculate_slippage(double avg_price, double benchmark_price) const { return ((avg_price - benchmark_price) / benchmark_price) * 10000.0; };

public:
    ExecutionEngine() = default; // Constructor by defaukt

    // Prices: contiguous in memory for low latency
    ExecutionResult execute_twap(std::vector<double>& prices, Order& order, size_t start_idx);

};

} // namespace execution