#include <gtest/gtest.h>
#include "execution_engine.hpp"

using namespace execution;


// Test basic execution
TEST(VWAPTest, BasicExecution) {
    // AAA
    // Arrange
    std::vector<double> prices = {100.0, 101.0, 102.0, 103.0, 104.0};
    std::vector<double> volumes = {100, 101, 102, 103, 104};

    Order order(1000.0, "buy", 5);
    ExecutionEngine engine;

    // Act
    ExecutionResult result = engine.execute_vwap(prices, volumes, order, 0);

    // Assert
    ASSERT_EQ(result.slices.size(), 5);
}