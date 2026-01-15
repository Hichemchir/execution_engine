#include "execution_engine.hpp"
#include "order.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>

namespace execution {

// Cut order in equal sub order
ExecutionResult ExecutionEngine::execute_twap(
    std::vector<double>& prices,
    Order& order,
    const size_t start_idx
) {
    ExecutionResult results;

    // cut into equal slices
    int slice_size = order.size / order.num_slices;
    double total_cost = 0.0;
    double avg_price = 0.0;
    size_t end_idx = std::min(start_idx + order.num_slices, prices.size());
    size_t actual_slices = end_idx - start_idx;

    // iteration over prices
    for (size_t i = start_idx; i < end_idx; ++i) {
        double price = prices[i];
        int day_idx = static_cast<int>(i - start_idx + 1);
        double cost = slice_size * price;
        total_cost += cost;
        avg_price += price;

        ExecutionSlice exec = ExecutionSlice(
            day_idx,
            slice_size,
            price,
            cost
        );

        results.slices.push_back(exec);
    }

    // Calculate metrics (benchmark is first price)
    double benchmark = prices[start_idx];
    results.benchmark_price = benchmark;
    results.avg_price = avg_price / actual_slices;
    results.slippage_bps = ExecutionEngine::calculate_slippage(results.avg_price, benchmark);

    return results;
};

ExecutionResult ExecutionEngine::execute_vwap(
    std::vector<double>& prices,
    std::vector<double>& volumes,
    Order& order,
    const size_t start_idx
) {
    ExecutionResult results;

    if (prices.size() != volumes.size()) {
        throw std::invalid_argument("Prices and volumes must have same size");
    }

    if (start_idx >= prices.size()) {
        throw std::out_of_range("Start_idx out of range");
    }

    // Execution window
    size_t end_idx = std::min(start_idx + order.num_slices, prices.size());
    size_t actual_slices = end_idx - start_idx;

    // Step 1: calculate total volume in execution window
    double total_volume = 0;
    for (size_t i = 0; i < end_idx; ++i) {
        total_volume += volumes[i];
    }

    // Step 2: calculate volume percentage and slice sizes
    std::vector<double> volume_pct(actual_slices);
    std::vector<int> slice_sizes(actual_slices);

    if (total_volume == 0.0) {
        // equal distrib if no volume data
        double equal_pct = 1.0 / actual_slices;
        for (size_t i = 0; i < actual_slices; ++i) {
            volume_pct[i] = equal_pct;
            slice_sizes[i] = order.size * equal_pct;
        }
    }
    else {
        // proportionnal to volume
        for (size_t i = 0; i < actual_slices; ++i) {
            size_t price_idx = start_idx + i;
            volume_pct[i] = volumes[price_idx] / total_volume;
            slice_sizes[i] = order.size * volume_pct[i];
        }
    }

    // step 3: execute slices
    double total_cost = 0.0;
    double weighted_price = 0.0;

    for (size_t i = 0; i < actual_slices; ++i) {
        size_t price_idx = start_idx + i;

        double price = prices[price_idx];
        double slice_size = slice_sizes[i];
        double cost = slice_size * price;

        total_cost += cost;
        weighted_price += price * slice_size;

        ExecutionSlice exec = ExecutionSlice(
            static_cast<int>(i + 1),
            static_cast<int>(slice_size),
            price,
            cost
        );

        results.slices.push_back(exec);
    }

    // Calculate metrics (benchmark is first price)
    double benchmark = prices[start_idx];
    results.benchmark_price = benchmark;
    results.avg_price = total_cost / order.size;
    results.slippage_bps = ExecutionEngine::calculate_slippage(results.avg_price, benchmark);

    return results;
};

} // namespace execution