#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <librdkafka/rdkafkacpp.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>

namespace trading {

// Market tick structure
strcut MarketTick {
    std::string symbol;
    double price;
    double volume;
    uint64_t timestamp;
    std::string exchange;

    // Serialize ton JSON for Kafka
    std::string to_json() const;
};

// Callback for tick events
using TickCallback = std::function<void(const MarketTick&)>;

// Feed handler configuration
struct FeedConfig {
    std::string api_key;
    std::string kafka_brokers = "localhost:9092";
    std::string kafka_topic = "market_ticks";
    std::vector<std::string> symbols;
    bool enable_kafka = true;
    bool enable_logging = true;
};

// Main Feed Handler class
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
    void unsubscribe(const std::string& symbol);
    
    // Register callback for tick events
    void on_tick(TickCallback callback);
    
    // Metrics
    struct Metrics {
        uint64_t ticks_received = 0;
        uint64_t ticks_dropped = 0;
        uint64_t kafka_sent = 0;
        uint64_t kafka_failed = 0;
        uint64_t reconnects = 0;
    };
    
    Metrics get_metrics() const;

private:
    void setup_websocket();
    void setup_kafka();
    void on_websocket_message(const ix::WebSocketMessagePtr& msg);
    void send_to_kafka(const MarketTick& tick);
    void heartbeat_loop();
    
    FeedConfig config_;
    std::unique_ptr<ix::WebSocket> websocket_;
    std::unique_ptr<RdKafka::Producer> kafka_producer_;
    
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    
    std::vector<TickCallback> callbacks_;
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;
};


} // namespace trading