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

} // namespace trading