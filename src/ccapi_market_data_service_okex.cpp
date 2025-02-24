#include "ccapi_okex/ccapi_market_data_service_okex.h"

namespace ccapi {
    MarketDataServiceOkex::MarketDataServiceOkex(
        std::function<void(Event&, Queue<Event>*)> eventHandler,
        SessionOptions sessionOptions,
        SessionConfigs sessionConfigs,
        ServiceContext* serviceContextPtr)
        : MarketDataService(eventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
    }

    MarketDataServiceOkex::~MarketDataServiceOkex() {
    }
}
