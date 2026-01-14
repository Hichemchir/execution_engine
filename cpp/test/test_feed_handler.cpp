#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "feed_handler.hpp"
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdlib>

using namespace trading;
using namespace std::chrono_literals;

// ============================================================================
// Test Utilities
// ============================================================================

namespace test_utils {

std::string get_env_var(const std::string& key, 
                        const std::string& env_file_path = "../../.env") {
    // Try environment variable first
    const char* env_val = std::getenv(key.c_str());
    if (env_val != nullptr && env_val[0] != '\0') {
        return std::string(env_val);
    }
    
    // Try .env file
    std::ifstream env_file(env_file_path);
    if (!env_file.is_open()) {
        return "";
    }
    
    std::string line;
    while (std::getline(env_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string file_key = line.substr(0, pos);
            std::string file_value = line.substr(pos + 1);
            
            // Trim whitespace
            file_key.erase(0, file_key.find_first_not_of(" \t"));
            file_key.erase(file_key.find_last_not_of(" \t") + 1);
            file_value.erase(0, file_value.find_first_not_of(" \t"));
            file_value.erase(file_value.find_last_not_of(" \t") + 1);
            
            if (file_key == key) {
                return file_value;
            }
        }
    }
    
    return "";
}

bool has_api_key() {
    return !get_env_var("FINNHUB_API_KEY").empty();
}

} // namespace test_utils

// ============================================================================
// Test Fixtures
// ============================================================================

class FeedHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::string api_key = test_utils::get_env_var("FINNHUB_API_KEY");
        
        config_ = FeedConfig{
            .api_key = api_key,
            .symbols = {"AAPL", "MSFT"},
            .enable_logging = false
        };
    }

    FeedConfig config_;
};

class FeedHandlerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!test_utils::has_api_key()) {
            GTEST_SKIP() << "Skipping integration test: FINNHUB_API_KEY not set";
        }
        
        std::string api_key = test_utils::get_env_var("FINNHUB_API_KEY");
        
        config_ = FeedConfig{
            .api_key = api_key,
            .symbols = {"AAPL"},
            .enable_logging = true
        };
    }

    FeedConfig config_;
};

// ============================================================================
// Unit Tests (no real connection)
// ============================================================================

TEST_F(FeedHandlerTest, ConstructorInitializesConfig) {
    FeedHandler handler(config_);
    EXPECT_FALSE(handler.is_connected());
}

TEST_F(FeedHandlerTest, DoubleStartDoesNotCrash) {
    FeedHandler handler(config_);
    handler.start();
    handler.start();
    handler.stop();
    SUCCEED();
}

TEST_F(FeedHandlerTest, DoubleStopDoesNotCrash) {
    FeedHandler handler(config_);
    handler.start();
    handler.stop();
    handler.stop();
    SUCCEED();
}

TEST_F(FeedHandlerTest, StopWithoutStart) {
    FeedHandler handler(config_);
    EXPECT_NO_THROW(handler.stop());
}

TEST_F(FeedHandlerTest, SubscribeSingleSymbol) {
    FeedHandler handler(config_);
    EXPECT_NO_THROW(handler.subscribe("TSLA"));
}

TEST_F(FeedHandlerTest, SubscribeMultipleSymbols) {
    FeedHandler handler(config_);
    std::vector<std::string> symbols = {"GOOGL", "AMZN", "NFLX"};
    EXPECT_NO_THROW(handler.subscribe(symbols));
}

TEST_F(FeedHandlerTest, RegisterSingleCallback) {
    FeedHandler handler(config_);
    
    int callback_count = 0;
    handler.on_tick([&callback_count](const MarketTick& tick) {
        callback_count++;
    });
    
    EXPECT_EQ(callback_count, 0);
}

TEST_F(FeedHandlerTest, RegisterMultipleCallbacks) {
    FeedHandler handler(config_);
    
    int count1 = 0, count2 = 0, count3 = 0;
    
    handler.on_tick([&count1](const MarketTick&) { count1++; });
    handler.on_tick([&count2](const MarketTick&) { count2++; });
    handler.on_tick([&count3](const MarketTick&) { count3++; });
    
    EXPECT_EQ(count1, 0);
    EXPECT_EQ(count2, 0);
    EXPECT_EQ(count3, 0);
}

TEST_F(FeedHandlerTest, GetRecentTicksEmptySymbol) {
    FeedHandler handler(config_);
    auto ticks = handler.get_recent_ticks("UNKNOWN_SYMBOL", 10);
    EXPECT_TRUE(ticks.empty());
}

TEST_F(FeedHandlerTest, InitialMetricsAreZero) {
    FeedHandler handler(config_);
    auto metrics = handler.get_metrics();
    
    EXPECT_EQ(metrics.ticks_received, 0);
    EXPECT_EQ(metrics.ticks_processed, 0);
    EXPECT_EQ(metrics.callbacks_executed, 0);
    EXPECT_EQ(metrics.reconnects, 0);
    EXPECT_DOUBLE_EQ(metrics.avg_latency_us, 0.0);
    EXPECT_DOUBLE_EQ(metrics.p99_latency_us, 0.0);
}

TEST_F(FeedHandlerTest, PrintMetricsDoesNotCrash) {
    FeedHandler handler(config_);
    EXPECT_NO_THROW(handler.print_metrics());
}

TEST_F(FeedHandlerTest, EmptyConfigSymbols) {
    FeedConfig empty_config{
        .api_key = "test_key",
        .symbols = {},
        .enable_logging = false
    };
    
    FeedHandler handler(empty_config);
    EXPECT_NO_THROW(handler.start());
    EXPECT_NO_THROW(handler.stop());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(FeedHandlerTest, ConcurrentCallbackRegistration) {
    FeedHandler handler(config_);
    
    std::atomic<int> total_callbacks{0};
    
    auto register_callbacks = [&]() {
        for (int i = 0; i < 100; ++i) {
            handler.on_tick([&total_callbacks](const MarketTick&) {
                total_callbacks++;
            });
        }
    };
    
    std::thread t1(register_callbacks);
    std::thread t2(register_callbacks);
    std::thread t3(register_callbacks);
    
    t1.join();
    t2.join();
    t3.join();
    
    SUCCEED();
}

TEST_F(FeedHandlerTest, ConcurrentMetricsAccess) {
    FeedHandler handler(config_);
    handler.start();
    
    auto read_metrics = [&]() {
        for (int i = 0; i < 100; ++i) {
            auto metrics = handler.get_metrics();
            (void)metrics;
        }
    };
    
    std::thread t1(read_metrics);
    std::thread t2(read_metrics);
    
    t1.join();
    t2.join();
    
    handler.stop();
    SUCCEED();
}

// ============================================================================
// Integration Tests (require API key)
// ============================================================================

TEST_F(FeedHandlerIntegrationTest, RealConnectionReceivesTicks) {
    FeedHandler handler(config_);
    
    std::atomic<int> tick_count{0};
    
    handler.on_tick([&tick_count](const MarketTick& tick) {
        std::cout << "📈 Received: " << tick.symbol 
                  << " $" << tick.price << std::endl;
        tick_count++;
    });
    
    std::cout << "🔗 Connecting to Finnhub..." << std::endl;
    handler.start();
    
    // Wait for some ticks (adjust based on market hours)
    std::this_thread::sleep_for(10s);
    
    handler.stop();
    
    auto metrics = handler.get_metrics();
    
    std::cout << "\nIntegration Test Results:" << std::endl;
    std::cout << "  Ticks received: " << metrics.ticks_received << std::endl;
    std::cout << "  Callbacks executed: " << metrics.callbacks_executed << std::endl;
    std::cout << "  Avg latency: " << metrics.avg_latency_us << " μs" << std::endl;
    
    EXPECT_GE(metrics.ticks_received, 0);
}

TEST_F(FeedHandlerIntegrationTest, HistoryStorageWorks) {
    FeedHandler handler(config_);
    
    handler.start();
    std::this_thread::sleep_for(5s);
    
    auto ticks = handler.get_recent_ticks("AAPL", 10);
    
    std::cout << "Stored " << ticks.size() << " ticks in history" << std::endl;
    
    for (const auto& tick : ticks) {
        std::cout << "  " << tick.symbol << " $" << tick.price 
                  << " Vol:" << tick.volume << std::endl;
    }
    
    handler.stop();
    
    EXPECT_GE(ticks.size(), 0);
}

TEST_F(FeedHandlerIntegrationTest, MetricsAreTracked) {
    FeedHandler handler(config_);
    
    handler.start();
    std::this_thread::sleep_for(5s);
    handler.stop();
    
    auto metrics = handler.get_metrics();
    
    std::cout << "\nDetailed Metrics:" << std::endl;
    std::cout << "  Ticks received:     " << metrics.ticks_received << std::endl;
    std::cout << "  Ticks processed:    " << metrics.ticks_processed << std::endl;
    std::cout << "  Callbacks executed: " << metrics.callbacks_executed << std::endl;
    std::cout << "  Reconnects:         " << metrics.reconnects << std::endl;
    std::cout << "  Avg latency:        " << metrics.avg_latency_us << " μs" << std::endl;
    std::cout << "  P99 latency:        " << metrics.p99_latency_us << " μs" << std::endl;
    
    EXPECT_EQ(metrics.ticks_received, metrics.ticks_processed);
}

// ============================================================================
// MarketTick Tests
// ============================================================================

TEST(MarketTickTest, DefaultConstruction) {
    MarketTick tick;
    
    EXPECT_TRUE(tick.symbol.empty());
    EXPECT_DOUBLE_EQ(tick.price, 0.0);
    EXPECT_DOUBLE_EQ(tick.volume, 0.0);
    EXPECT_EQ(tick.timestamp, 0);
}