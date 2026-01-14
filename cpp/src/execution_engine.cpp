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
    results.slippage_bps = ExecutionEngine::calculate_slippage(avg_price, benchmark);

    return results;
};

ExecutionResult ExecutionEngine::execute_vwap(
    std::vector<double>& prices,
    std::vector<double>& volumes,
    Order& order,
    const size_t start_idx
) {
    ExecutionResult results;

    std::vector<double> volume_pct;
    std::vector<int> slice_size;

    double total_volume = std::accumulate(volumes.begin(), volumes.end(), 0.0);
    if (total_volume == 0.0) {
        volume_pct = 1 / volumes.size();
    }
    else {
        volume_pct = volumes / total_volume;
    }

    slice_size = volume_pct * order.size;

    std::vector<ExecutionSlice> slices;
    double total_cost = 0.0;
    size_t end_idx = std::min(start_idx + order.num_slices, prices.size())


    // iteration over prices
    for (size_t i = start_idx; i < end_idx; ++i) {
        double price = prices[i];
        int day_idx = i + 1;
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
    double benchmark = prices[0];
    results.benchmark_price = benchmark;
    results.avg_price = avg_price / prices.size();
    results.slippage_bps = ExecutionEngine::calculate_slippage(avg_price, benchmark);

    return results;
};

} // namespace execution