#include "feed_handler.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace trading {

FeedHandler::FeedHandler(const FeedConfig& config)
    : config_(config)
{
    setup_websocket();
}

FeedHandler::~FeedHandler() {
    stop();
}

void FeedHandler::setup_websocket() {
    websocket_ = std::make_unique<ix::WebSocket>();
    
    std::string url = "wss://ws.finnhub.io/?token=" + config_.api_key;
    websocket_->setUrl(url);

    websocket_->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        on_websocket_message(msg);
    });

    if (config_.enable_logging) {
        std::cout << "📡 WebSocket configured for Finnhub\n";
    }
}

void FeedHandler::start() {
    if (running_) return;  // Already running

    running_ = true;
    websocket_->start();

    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Subscribe to configured symbols
    if (!config_.symbols.empty()) {
        subscribe(config_.symbols);
    }

    // Start heartbeat
    heartbeat_thread_ = std::thread(&FeedHandler::heartbeat_loop, this);

    if (config_.enable_logging) {
        std::cout << "🚀 Feed handler started\n";
    }
}

void FeedHandler::stop() {
    if (!running_) return;  // ✅ FIXED: Already stopped
    
    running_ = false;
    connected_ = false;

    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    websocket_->stop();

    if (config_.enable_logging) {
        std::cout << "🛑 Feed handler stopped\n";
        print_metrics();
    }
}

void FeedHandler::subscribe(const std::string& symbol) {
    nlohmann::json msg = {
        {"type", "subscribe"},
        {"symbol", symbol}
    };
    websocket_->send(msg.dump());
    
    if (config_.enable_logging) {
        std::cout << "📊 Subscribed to " << symbol << "\n";
    }
}

void FeedHandler::subscribe(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        subscribe(symbol);
    }
}

void FeedHandler::on_tick(TickCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    callbacks_.push_back(callback);
}

void FeedHandler::on_websocket_message(const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Open) {
        connected_ = true;
        if (config_.enable_logging) {
            std::cout << "🔗 Connected to Finnhub\n";
        }
        return;
    }
    
    if (msg->type == ix::WebSocketMessageType::Close) {
        connected_ = false;
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.reconnects++;
        return;
    }
    
    if (msg->type == ix::WebSocketMessageType::Error) {
        if (config_.enable_logging) {
            std::cerr << "❌ WebSocket error: " << msg->errorInfo.reason << "\n";
        }
        return;
    }
    
    if (msg->type != ix::WebSocketMessageType::Message) {
        return;
    }
    
    // Start latency measurement
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        auto data = nlohmann::json::parse(msg->str);
        
        if (!data.contains("type") || data["type"] != "trade") {
            return;
        }
        
        if (!data.contains("data") || !data["data"].is_array()) {
            return;
        }
        
        for (const auto& trade : data["data"]) {
            MarketTick tick;
            tick.symbol = trade["s"];
            tick.price = trade["p"];
            tick.volume = trade["v"];
            tick.timestamp = trade["t"];
            tick.exchange = "FINNHUB";
            
            {
                std::lock_guard<std::mutex> lock(metrics_mutex_);
                metrics_.ticks_received++;
            }
            
            process_tick(tick);
        }
        
    } catch (const std::exception& e) {
        if (config_.enable_logging) {
            std::cerr << "⚠️  Error: " << e.what() << "\n";
        }
    }
    
    // Measure latency
    auto end = std::chrono::high_resolution_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    update_latency(static_cast<double>(latency_us));
}

void FeedHandler::process_tick(const MarketTick& tick) {
    // Store in history
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        auto& history = tick_history_[tick.symbol];
        history.push_back(tick);
        
        if (history.size() > MAX_HISTORY_PER_SYMBOL) {
            history.pop_front();
        }
    }
    
    // Execute callbacks
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        for (const auto& callback : callbacks_) {
            try {
                callback(tick);
                
                std::lock_guard<std::mutex> mlock(metrics_mutex_);
                metrics_.callbacks_executed++;
            } catch (const std::exception& e) {
                std::cerr << "⚠️  Callback error: " << e.what() << "\n";
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.ticks_processed++;
    }
}

std::vector<MarketTick> FeedHandler::get_recent_ticks(const std::string& symbol, size_t count) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    auto it = tick_history_.find(symbol);
    if (it == tick_history_.end()) {
        return {};
    }
    
    const auto& history = it->second;
    size_t start = history.size() > count ? history.size() - count : 0;
    
    std::vector<MarketTick> result;
    for (size_t i = start; i < history.size(); ++i) {
        result.push_back(history[i]);
    }
    
    return result;
}

void FeedHandler::update_latency(double latency_us) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    latency_samples_.push_back(latency_us);
    
    // Keep only last 10000 samples
    if (latency_samples_.size() > 10000) {
        latency_samples_.erase(latency_samples_.begin());
    }
    
    // Update average
    double sum = 0;
    for (double l : latency_samples_) {
        sum += l;
    }
    metrics_.avg_latency_us = sum / latency_samples_.size();
    
    // Calculate P99
    auto sorted = latency_samples_;
    std::sort(sorted.begin(), sorted.end());
    size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
    metrics_.p99_latency_us = sorted[p99_idx];
}

void FeedHandler::heartbeat_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        if (running_ && connected_ && config_.enable_logging) {
            print_metrics();
        }
    }
}

FeedHandler::Metrics FeedHandler::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void FeedHandler::print_metrics() const {
    auto m = get_metrics();
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "📊 Feed Handler Metrics\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "  Ticks received:     " << m.ticks_received << "\n";
    std::cout << "  Ticks processed:    " << m.ticks_processed << "\n";
    std::cout << "  Callbacks executed: " << m.callbacks_executed << "\n";
    std::cout << "  Reconnects:         " << m.reconnects << "\n";
    std::cout << "  Avg latency:        " << std::fixed << std::setprecision(2) 
              << m.avg_latency_us << " μs\n";
    std::cout << "  P99 latency:        " << std::fixed << std::setprecision(2) 
              << m.p99_latency_us << " μs\n";
    std::cout << std::string(60, '=') << "\n\n";
}

} // namespace trading