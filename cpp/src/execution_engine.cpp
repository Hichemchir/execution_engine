#include "execution_engine.hpp"
#include "order.hpp"

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
    double total_cost;
    double avg_price;
    size_t end_idx = std::min(start_idx + order.num_slices, prices.size());

    // iteration over prices
    for (size_t i = start_idx; i < end_idx; ++i) {
        double price = prices[i];
        int day_idx = start_idx + 1;
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