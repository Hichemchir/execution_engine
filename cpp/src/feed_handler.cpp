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

// Destructor
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
        std::cout << "WebSocket configured for Finnhub\n";
    }
}

void FeedHandler::start() {
    if (running_) return;

    running_ = true;
    websocket_->start();

    // WAit for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Subscribe to configured symbols
    if (!config_.symbols.empty()) {
        subscribe(config_.symbols);
    }

    // Start heartbeat
    heartbeat_thread_ = std::thread(&FeedHandler::heartbeat_loop, this);

    if (config_.enable_logging) {
        std::cout << "Feed handler started\n";
    }
}

void FeedHandler::stop() {
    if (running_) return;
    running_ = false;

    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    websocket_->stop();

    if (config_.enable_logging) {
        std::cout << "Feed handler stopped\n";
        print_metrics();
    }
}

} // namespace trading