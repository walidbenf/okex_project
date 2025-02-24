/**
 * @file test_execution_management_service_okex.cpp
 * @brief Tests unitaires pour le service de gestion des ordres OKX
 *
 * Teste les fonctionnalités principales :
 * - Création d'ordres avec identifiant client (clOrdId)
 * - Annulation d'ordres par lots via identifiants client
 */

#include "../include/ccapi_okex/ccapi_execution_management_service_okex.h"
#include <cassert>
#include <iostream>

using namespace ccapi;

const std::map<std::string, std::string> credentials = {
    {"CCAPI_OKX_API_KEY", "5137e278-1f35-4ddb-8452-76756fd9ff6c"},
    {"CCAPI_OKX_API_SECRET", "EFFF4539ADCD144C01BC58CDDFDA76B8"},
    {"CCAPI_OKX_API_PASSPHRASE", "Walidtest123."}
};

/**
 * @brief Teste la création d'un ordre avec un identifiant client
 *
 * Vérifie :
 * - La méthode HTTP (POST)
 * - L'endpoint correct (/api/v5/trade/order)
 * - Le format du corps de la requête avec clOrdId
 * - La présence des headers d'authentification
 */
void testCreateOrderWithClientOrderId() {
    SessionOptions sessionOptions;
    SessionConfigs sessionConfigs;
    ServiceContext* serviceContextPtr = new ServiceContext();

    ExecutionManagementServiceOkex service(nullptr, sessionOptions, sessionConfigs, serviceContextPtr);

    Request request(Request::Operation::CREATE_ORDER, "okx", "BTC-USDT");
    std::map<std::string, std::string> params = {
        {"SIDE", "buy"},
        {"ORDER_TYPE", "limit"},
        {"QUANTITY", "0.1"},
        {"PRICE", "50000"},
        {"CLIENT_ORDER_ID", "test_order_123"}
    };
    request.appendParam(params);

    http::request<http::string_body> req;
    service.convertRequestForRest(
        req,
        request,
        "wsid",
        TimePoint(std::chrono::milliseconds(1234567890000)),
        "BTC-USDT",
        credentials
    );

    // Vérifications
    assert(req.method_string() == "POST");
    assert(req.target() == "/api/v5/trade/order");

    std::string expectedBody = R"({"instId":"BTC-USDT","tdMode":"cash","side":"buy","ordType":"limit","sz":"0.1","clOrdId":"test_order_123","px":"50000"})";
    assert(req.body() == expectedBody);

    assert(req.base().at("Content-Type") == "application/json");
    assert(req.base().find("OK-ACCESS-KEY") != req.base().end());
    assert(req.base().find("OK-ACCESS-SIGN") != req.base().end());
    assert(req.base().find("OK-ACCESS-TIMESTAMP") != req.base().end());
    assert(req.base().find("OK-ACCESS-PASSPHRASE") != req.base().end());

    std::cout << "Test create order with client ID passed!" << std::endl;

    delete serviceContextPtr;
}

/**
 * @brief Teste l'annulation par lots d'ordres via leurs identifiants client
 *
 * Vérifie :
 * - La méthode HTTP (POST)
 * - L'endpoint correct (/api/v5/trade/cancel-batch-orders)
 * - Le format du corps de la requête avec tableau d'ordres
 * - La présence des headers d'authentification
 *
 * Format attendu du corps :
 * {
 *   "orders": [
 *     {"clOrdId": "order1", "instId": "BTC-USDT"},
 *     {"clOrdId": "order2", "instId": "BTC-USDT"},
 *     {"clOrdId": "order3", "instId": "BTC-USDT"}
 *   ]
 * }
 */
void testCancelOrdersByClientOrderIds() {
    SessionOptions sessionOptions;
    SessionConfigs sessionConfigs;
    ServiceContext* serviceContextPtr = new ServiceContext();

    ExecutionManagementServiceOkex service(nullptr, sessionOptions, sessionConfigs, serviceContextPtr);

    Request request(Request::Operation::CANCEL_OPEN_ORDERS, "okx", "BTC-USDT");
    request.appendParam({{"clOrdIds", "order1,order2,order3"}});

    http::request<http::string_body> req;
    service.convertRequestForRest(
        req,
        request,
        "wsid",
        TimePoint(std::chrono::milliseconds(1234567890000)),
        "BTC-USDT",
        credentials
    );

    // Vérifications
    assert(req.method_string() == "POST");
    assert(req.target() == "/api/v5/trade/cancel-batch-orders");

    std::string expectedBody = R"({"orders":[{"clOrdId":"order1","instId":"BTC-USDT"},{"clOrdId":"order2","instId":"BTC-USDT"},{"clOrdId":"order3","instId":"BTC-USDT"}]})";

    // debug
    std::cout << "Expected body: " << expectedBody << std::endl;
    std::cout << "Actual body: " << req.body() << std::endl;

    assert(req.body() == expectedBody);

    assert(req.base().find("OK-ACCESS-KEY") != req.base().end());
    assert(req.base().find("OK-ACCESS-SIGN") != req.base().end());
    assert(req.base().find("OK-ACCESS-TIMESTAMP") != req.base().end());
    assert(req.base().find("OK-ACCESS-PASSPHRASE") != req.base().end());

    std::cout << "Test cancel orders by client IDs passed!" << std::endl;

    delete serviceContextPtr;
}

/**
 * @brief Teste la création d'ordre avec des paramètres invalides
 */
void testCreateOrderWithInvalidParams() {
    SessionOptions sessionOptions;
    SessionConfigs sessionConfigs;
    ServiceContext* serviceContextPtr = new ServiceContext();

    ExecutionManagementServiceOkex service(nullptr, sessionOptions, sessionConfigs, serviceContextPtr);

    // Test avec quantité négative
    Request request1(Request::Operation::CREATE_ORDER, "okx", "BTC-USDT");
    std::map<std::string, std::string> params1 = {
        {"SIDE", "buy"},
        {"ORDER_TYPE", "limit"},
        {"QUANTITY", "-0.1"},  // Quantité invalide
        {"PRICE", "50000"},
        {"CLIENT_ORDER_ID", "test_order_123"}
    };
    request1.appendParam(params1);

    try {
        http::request<http::string_body> req;
        service.convertRequestForRest(req, request1, "wsid",
            TimePoint(std::chrono::milliseconds(1234567890000)),
            "BTC-USDT",
            credentials);
        assert(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
        std::cout << "Test invalid quantity caught expected error: " << e.what() << std::endl;
    }

    // Test avec prix négatif
    Request request2(Request::Operation::CREATE_ORDER, "okx", "BTC-USDT");
    std::map<std::string, std::string> params2 = {
        {"SIDE", "buy"},
        {"ORDER_TYPE", "limit"},
        {"QUANTITY", "0.1"},
        {"PRICE", "-50000"},  // Prix invalide
        {"CLIENT_ORDER_ID", "test_order_123"}
    };
    request2.appendParam(params2);

    try {
        http::request<http::string_body> req;
        service.convertRequestForRest(req, request2, "wsid",
            TimePoint(std::chrono::milliseconds(1234567890000)),
            "BTC-USDT",
            credentials);
        assert(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
        std::cout << "Test invalid price caught expected error: " << e.what() << std::endl;
    }

    delete serviceContextPtr;
    std::cout << "Test create order with invalid params passed!" << std::endl;
}

/**
 * @brief Teste l'annulation d'ordres avec des identifiants invalides
 */
void testCancelOrdersWithInvalidIds() {
    SessionOptions sessionOptions;
    SessionConfigs sessionConfigs;
    ServiceContext* serviceContextPtr = new ServiceContext();

    ExecutionManagementServiceOkex service(nullptr, sessionOptions, sessionConfigs, serviceContextPtr);

    // Test avec liste vide
    Request request1(Request::Operation::CANCEL_OPEN_ORDERS, "okx", "BTC-USDT");
    request1.appendParam({{"clOrdIds", ""}});

    try {
        http::request<http::string_body> req;
        service.convertRequestForRest(req, request1, "wsid",
            TimePoint(std::chrono::milliseconds(1234567890000)),
            "BTC-USDT",
            credentials);
        assert(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
        std::cout << "Test empty clOrdIds caught expected error: " << e.what() << std::endl;
    }

    delete serviceContextPtr;
    std::cout << "Test cancel orders with invalid IDs passed!" << std::endl;
}

/**
 * @brief Point d'entrée des tests
 *
 * Exécute tous les tests unitaires et affiche les résultats.
 * En cas d'échec d'un test, affiche l'erreur et retourne un code d'erreur.
 *
 * @return 0 si tous les tests passent, 1 en cas d'erreur
 */
int main() {
    try {
        testCreateOrderWithClientOrderId();
        testCancelOrdersByClientOrderIds();
        testCreateOrderWithInvalidParams();
        testCancelOrdersWithInvalidIds();
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
}
