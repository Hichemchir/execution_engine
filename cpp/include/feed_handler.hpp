#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>

namespace trading {

// Market tick structure
struct MarketTick {
    std::string symbol;
    double price;
    double volume;
    uint64_t timestamp;
    std::string exchange;

    // Serialize ton JSON for Kafka
    std::string to_json() const {
        return symbol + " $" + std::to_string(price) + 
                " Vol:" + std::to_string(volume);
    }
};

// Callback for tick events
using TickCallback = std::function<void(const MarketTick&)>;

// Feed handler configuration
struct FeedConfig {
    std::string api_key;
    std::vector<std::string> symbols;
    bool enable_logging = true;
};

// Main Feed Handler class (IN MEMORY)
class FeedHandler {
public:
    explicit FeedHandler(const FeedConfig& config);
    ~FeedHandler();

    // Lifecycle
    void start();
    void stop();
    bool is_connected() const { return connected_; }

    // Subscribe to symbols
    void subscribe(const std::string& symbol);
    void subscribe(const std::vector<std::string>& symbols);
    
    // Register callback for tick events
    void on_tick(TickCallback callback);

    // Get recent ticks (for backtesting/analysis)
    std::vector<MarketTick> get_recent_ticks(const std::string& symbol, size_t count = 100);
    
    // Metrics
    struct Metrics {
        uint64_t ticks_received = 0;
        uint64_t ticks_processed = 0;
        uint64_t callbacks_executed = 0;
        uint64_t reconnects = 0;
        double avg_latency_us = 0.0;
        double p99_latency_us = 0.0;
    };
    
    Metrics get_metrics() const;
    void print_metrics() const;

private:
    void setup_websocket();
    void on_websocket_message(const ix::WebSocketMessagePtr& msg);
    void process_tick(const MarketTick& tick);
    void heartbeat_loop();
    void update_latency(double latency_us);
    
    FeedConfig config_;
    std::unique_ptr<ix::WebSocket> websocket_;
    
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    
    // Callbacks
    std::vector<TickCallback> callbacks_;
    std::mutex callbacks_mutex_;
    
    // In-memory tick storage (per symbol)
    std::unordered_map<std::string, std::deque<MarketTick>> tick_history_;
    std::mutex history_mutex_;
    static constexpr size_t MAX_HISTORY_PER_SYMBOL = 10000;
    
    // Metrics
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;
    std::vector<double> latency_samples_;
};


} // namespace trading