#include <gtest/gtest.h>
#include "execution_engine.hpp"

using namespace execution;

// Test 1
TEST(TWAPTest, orderCreation) {
    Order order(100.0, "buy", 10);

    ASSERT_EQ(order.num_slices, 10);
}

// Test basic execution
TEST(TWAPTest, BasicExecution) {
    // AAA
    // Arrange
    std::vector<double> prices = {100.0, 101.0, 102.0, 103.0, 104.0};
    Order order(1000.0, "buy", 5);
    ExecutionEngine engine;

    // Act
    ExecutionResult result = engine.execute_twap(prices, order, 0);

    // Assert
    ASSERT_EQ(result.slices.size(), 5);
    EXPECT_DOUBLE_EQ(result.benchmark_price, 100.0);
}