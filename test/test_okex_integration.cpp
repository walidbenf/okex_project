#include "../include/ccapi_okex/ccapi_execution_management_service_okex.h"
#include <cassert>
#include <iostream>
#include <thread>

using namespace ccapi;

// Ajouter les credentials
const std::map<std::string, std::string> credentials = {
    {"CCAPI_OKX_API_KEY", "5137e278-1f35-4ddb-8452-76756fd9ff6c"},
    {"CCAPI_OKX_API_SECRET", "EFFF4539ADCD144C01BC58CDDFDA76B8"},
    {"CCAPI_OKX_API_PASSPHRASE", "Walidtest123."}
};

void handleEvent(Event& event, Queue<Event>* eventQueue) {
    std::cout << "Received event: " << event.toStringPretty(2, 2) << std::endl;
}

int main() {
    try {
        // Configuration
        SessionOptions sessionOptions;
        SessionConfigs sessionConfigs;
        ServiceContext* serviceContextPtr = new ServiceContext();

        // Création du service avec le handler d'événements
        ExecutionManagementServiceOkex service(handleEvent, sessionOptions, sessionConfigs, serviceContextPtr);

        // Création d'un ordre avec clOrdId
        Request request(Request::Operation::CREATE_ORDER, "okx", "BTC-USDT");
        std::map<std::string, std::string> params = {
            {"SIDE", "buy"},
            {"ORDER_TYPE", "limit"},
            {"QUANTITY", "0.001"},  // Petite quantité pour test
            {"PRICE", "25000"},     // Prix raisonnable
            {"CLIENT_ORDER_ID", "test_integration_" + std::to_string(std::time(nullptr))}
        };
        request.appendParam(params);

        // Envoi de la requête avec les bons paramètres
        TimePoint now = std::chrono::system_clock::now();
        Queue<Event>* eventQueue = nullptr;
        service.sendRequest(request, false, now, 0, eventQueue);

        // Attente pour la réponse
        std::this_thread::sleep_for(std::chrono::seconds(2));

        delete serviceContextPtr;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
}
