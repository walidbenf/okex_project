#include "../include/ccapi_okex/ccapi_execution_management_service_okex.h"
#include "../include/ccapi_cpp/ccapi_session.h"
#include "../include/ccapi_cpp/ccapi_logger.h"
#include "../include/ccapi_cpp/ccapi_macro.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>

namespace ccapi {
Logger* Logger::logger = nullptr;  // Nécessaire pour l'initialisation du logger

class MyEventHandler : public EventHandler {
 public:
  MyEventHandler(std::mutex& mtx, std::condition_variable& cv, bool& response_received, bool& order_success)
    : mtx_(mtx), cv_(cv), response_received_(response_received), order_success_(order_success) {}

  bool processEvent(const Event& event, Session* session) override {
    std::cout << "Received event: " << event.typeToString(event.getType()) << std::endl;
    std::cout << "Full event details: " << event.toStringPretty(2, 2) << std::endl;

    if (event.getType() == Event::Type::RESPONSE) {
        const auto& messageList = event.getMessageList();
        if (!messageList.empty()) {
            const auto& message = messageList[0];
            std::cout << "Message content: " << message.toStringPretty(2, 2) << std::endl;

            std::unique_lock<std::mutex> lock(mtx_);
            order_success_ = (message.getType() != Message::Type::RESPONSE_ERROR);
            response_received_ = true;
            cv_.notify_one();
        }
    } else if (event.getType() == Event::Type::REQUEST_STATUS) {
        std::cout << "Request status received" << std::endl;
    }
    return true;
  }

 private:
  std::mutex& mtx_;
  std::condition_variable& cv_;
  bool& response_received_;
  bool& order_success_;
};
} /* namespace ccapi */

using namespace ccapi;

int main() {
    try {
        std::cout << "Initializing test..." << std::endl;

        // Variables pour la synchronisation
        std::mutex mtx;
        std::condition_variable cv;
        bool response_received = false;
        bool order_success = false;

        // Configuration
        SessionOptions sessionOptions;
        sessionOptions.enableOneHttpConnectionPerRequest = true;

        SessionConfigs sessionConfigs;
        std::map<std::string, std::string> urlRestBase = {
            {CCAPI_EXCHANGE_NAME_OKX, "https://www.okx.com/api/v5"}
        };
        sessionConfigs.setUrlRestBase(urlRestBase);

        // Activer le service d'exécution pour OKX
        Subscription subscription(CCAPI_EXCHANGE_NAME_OKX,  // exchange
                                "BTC-USDT",                 // instrument
                                CCAPI_EXECUTION_MANAGEMENT, // field
                                "",                         // options
                                "",                         // correlationId
                                {});                        // messageType

        std::cout << "Creating session..." << std::endl;
        MyEventHandler eventHandler(mtx, cv, response_received, order_success);
        Session session(sessionOptions, sessionConfigs, &eventHandler);

        session.subscribe(subscription);

        std::cout << "Creating request..." << std::endl;
        Request request(Request::Operation::CREATE_ORDER, CCAPI_EXCHANGE_NAME_OKX, "BTC-USDT");

        request.appendParam({
            {"instId", "BTC-USDT"},
            {"tdMode", "cash"},
            {"side", "buy"},
            {"ordType", "limit"},
            {"sz", "0.001"},
            {"px", "25000"}
        });

        // Configuration des credentials
        std::map<std::string, std::string> credential = {
            {CCAPI_OKX_API_KEY, "5137e278-1f35-4ddb-8452-76756fd9ff6c"},
            {CCAPI_OKX_API_SECRET, "EFFF4539ADCD144C01BC58CDDFDA76B8"},
            {CCAPI_OKX_API_PASSPHRASE, "Walidtest123."},
            {CCAPI_OKX_API_X_SIMULATED_TRADING, "1"}
        };
        request.setCredential(credential);

        std::cout << "Sending request to OKX..." << std::endl;
        session.sendRequest(request);
        std::cout << "Request sent, waiting for response..." << std::endl;

        // Attente de la réponse
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (!cv.wait_for(lock, std::chrono::seconds(30), [&response_received]{ return response_received; })) {
                std::cerr << "Timeout waiting for response" << std::endl;
                session.stop();
                return 1;
            }
        }

        std::cout << "Stopping session..." << std::endl;
        session.stop();
        std::cout << "Test completed" << std::endl;

        return order_success ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
}
