#ifndef CCAPI_EXECUTION_MANAGEMENT_SERVICE_OKEX_H
#define CCAPI_EXECUTION_MANAGEMENT_SERVICE_OKEX_H

#include "../ccapi_cpp/service/ccapi_execution_management_service.h"
#include "../ccapi_cpp/ccapi_util.h"  // Pour UtilAlgorithm

namespace ccapi {
    /**
     * @brief Service de gestion des ordres pour OKX
     * Implémente les opérations de trading sur OKX, incluant :
     * - Création d'ordres avec identifiants client (clOrdId)
     * - Annulation d'ordres par lots via identifiants client
     */
    class ExecutionManagementServiceOkex : public ExecutionManagementService {
    public:
        /**
         * @brief Constructeur du service
         * @param eventHandler Gestionnaire d'événements pour les réponses
         * @param sessionOptions Options de la session
         * @param sessionConfigs Configuration de la session
         * @param serviceContextPtr Contexte du service
         */
        ExecutionManagementServiceOkex(std::function<void(Event&, Queue<Event>*)> eventHandler,
                                     SessionOptions sessionOptions,
                                     SessionConfigs sessionConfigs,
                                     ServiceContextPtr serviceContextPtr)
            : ExecutionManagementService(eventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
            this->exchangeName = CCAPI_EXCHANGE_NAME_OKX;
            this->baseUrlWs = sessionConfigs.getUrlWebsocketBase().at(this->exchangeName) + "/ws/v5/private";
            this->baseUrlRest = sessionConfigs.getUrlRestBase().at(this->exchangeName);

            // Configuration des endpoints
            this->createOrderTarget = "/api/v5/trade/order";
            this->cancelOrderTarget = "/api/v5/trade/cancel-order";
            this->cancelBatchOrdersTarget = "/api/v5/trade/cancel-batch-orders";

            // Configuration de l'authentification
            this->apiKeyName = CCAPI_OKX_API_KEY;
            this->apiSecretName = CCAPI_OKX_API_SECRET;
            this->apiPassphraseName = CCAPI_OKX_API_PASSPHRASE;
            this->apiXSimulatedTradingName = CCAPI_OKX_API_X_SIMULATED_TRADING;

            this->setupCredential({
                this->apiKeyName,
                this->apiSecretName,
                this->apiPassphraseName,
                this->apiXSimulatedTradingName
            });
        }

        /**
         * @brief Convertit une requête en format REST pour l'API OKX
         *
         * Gère deux opérations principales :
         * 1. CREATE_ORDER : Création d'ordre avec support de clOrdId
         * 2. CANCEL_OPEN_ORDERS : Annulation par lots avec clOrdIds
         *
         * Format de requête pour CREATE_ORDER :
         * {
         *   "instId": "BTC-USDT",
         *   "tdMode": "cash",
         *   "side": "buy",
         *   "ordType": "limit",
         *   "sz": "0.1",
         *   "clOrdId": "test-order-123",  // Optionnel
         *   "px": "50000"                 // Pour ordres limites
         * }
         *
         * Format pour CANCEL_OPEN_ORDERS :
         * {
         *   "orders": [
         *     {"clOrdId": "order1", "instId": "BTC-USDT"},
         *     {"clOrdId": "order2", "instId": "BTC-USDT"}
         *   ]
         * }
         */
        void convertRequestForRest(http::request<http::string_body>& req, const Request& request,
                                 const std::string& wsRequestId, const TimePoint& now,
                                 const std::string& symbolId,
                                 const std::map<std::string, std::string>& credential) override {
            switch (request.getOperation()) {
                case Request::Operation::CREATE_ORDER: {
                    req.method(http::verb::post);
                    req.target(this->createOrderTarget);

                    const std::map<std::string, std::string>& param = request.getFirstParamWithDefault();

                    // Vérification des paramètres obligatoires
                    std::vector<std::string> requiredParams = {"SIDE", "ORDER_TYPE", "QUANTITY"};
                    for (const auto& paramName : requiredParams) {
                        if (param.find(paramName) == param.end()) {
                            throw std::runtime_error("Missing required parameter: " + paramName);
                        }
                    }

                    // Validation des paramètres
                    if (param.find("QUANTITY") != param.end()) {
                        double quantity = std::stod(param.at("QUANTITY"));
                        if (quantity <= 0) {
                            throw std::runtime_error("Invalid quantity: must be positive");
                        }
                    }

                    // Validation du prix pour les ordres limites
                    if (param.at("ORDER_TYPE") == "limit") {
                        if (param.find("PRICE") == param.end()) {
                            throw std::runtime_error("Price is required for limit orders");
                        }
                        double price = std::stod(param.at("PRICE"));
                        if (price <= 0) {
                            throw std::runtime_error("Invalid price: must be positive");
                        }
                    }

                    // Validation du SIDE
                    if (param.at("SIDE") != "buy" && param.at("SIDE") != "sell") {
                        throw std::runtime_error("Invalid side: must be 'buy' or 'sell'");
                    }

                    rj::Document document;
                    document.SetObject();
                    rj::Document::AllocatorType& allocator = document.GetAllocator();

                    // Paramètres obligatoires
                    document.AddMember("instId", rj::Value(symbolId.c_str(), allocator).Move(), allocator);
                    document.AddMember("tdMode", rj::Value("cash", allocator).Move(), allocator);  // Mode cash par défaut
                    document.AddMember("side", rj::Value(param.at("SIDE").c_str(), allocator).Move(), allocator);
                    document.AddMember("ordType", rj::Value(param.at("ORDER_TYPE").c_str(), allocator).Move(), allocator);
                    document.AddMember("sz", rj::Value(param.at("QUANTITY").c_str(), allocator).Move(), allocator);

                    // Ajout du clOrdId s'il est fourni
                    if (param.find("CLIENT_ORDER_ID") != param.end()) {
                        document.AddMember("clOrdId", rj::Value(param.at("CLIENT_ORDER_ID").c_str(), allocator).Move(), allocator);
                    }

                    // Prix pour les ordres limites
                    if (param.find("PRICE") != param.end()) {
                        document.AddMember("px", rj::Value(param.at("PRICE").c_str(), allocator).Move(), allocator);
                    }

                    rj::StringBuffer stringBuffer;
                    rj::Writer<rj::StringBuffer> writer(stringBuffer);
                    document.Accept(writer);
                    auto body = stringBuffer.GetString();

                    req.body() = body;
                    req.set(http::field::content_type, "application/json");
                    this->signRequest(req, now, credential, body, this->apiXSimulatedTradingName);
                } break;

                case Request::Operation::CANCEL_OPEN_ORDERS: {
                    req.method(http::verb::post);
                    req.target(this->cancelBatchOrdersTarget);

                    const std::map<std::string, std::string>& param = request.getFirstParamWithDefault();

                    // Validation des identifiants
                    if (param.find("clOrdIds") == param.end() || param.at("clOrdIds").empty()) {
                        throw std::runtime_error("Invalid clOrdIds: must not be empty");
                    }

                    rj::Document document;
                    document.SetObject();
                    rj::Document::AllocatorType& allocator = document.GetAllocator();

                    // Créer le tableau d'ordres
                    rj::Value ordersArray(rj::kArrayType);

                    // Splitter la chaîne des clOrdIds
                    std::string clOrdIdsStr = param.at("clOrdIds");
                    std::vector<std::string> clOrdIds;
                    std::stringstream ss(clOrdIdsStr);
                    std::string clOrdId;
                    while (std::getline(ss, clOrdId, ',')) {
                        rj::Value orderObject(rj::kObjectType);
                        orderObject.AddMember("clOrdId", rj::Value(clOrdId.c_str(), allocator).Move(), allocator);
                        orderObject.AddMember("instId", rj::Value(symbolId.c_str(), allocator).Move(), allocator);
                        ordersArray.PushBack(orderObject, allocator);
                    }

                    document.AddMember("orders", ordersArray, allocator);

                    rj::StringBuffer stringBuffer;
                    rj::Writer<rj::StringBuffer> writer(stringBuffer);
                    document.Accept(writer);
                    auto body = stringBuffer.GetString();

                    req.body() = body;
                    req.set(http::field::content_type, "application/json");
                    this->signRequest(req, now, credential, body, this->apiXSimulatedTradingName);
                } break;

                default:
                    ExecutionManagementService::convertRequestForRest(req, request, wsRequestId, now, symbolId, credential);
                    break;
            }
        }

    protected:
        /**
         * @brief Signe une requête pour l'API OKX
         *
         * Ajoute les headers d'authentification requis :
         * - OK-ACCESS-KEY : Clé API
         * - OK-ACCESS-SIGN : Signature HMAC SHA256 en Base64
         * - OK-ACCESS-TIMESTAMP : Timestamp en millisecondes
         * - OK-ACCESS-PASSPHRASE : Passphrase API
         * - x-simulated-trading : Pour le compte de test
         *
         * @param req Requête à signer
         * @param now Timestamp actuel
         * @param credential Credentials API
         * @param body Corps de la requête
         * @param xSimulatedTrading Flag pour compte de test
         */
        void signRequest(http::request<http::string_body>& req, const TimePoint& now,
                        const std::map<std::string, std::string>& credential, const std::string& body,
                        const std::string& xSimulatedTrading) {
            auto apiKey = mapGetWithDefault(credential, this->apiKeyName);
            auto apiSecret = mapGetWithDefault(credential, this->apiSecretName);
            auto apiPassphrase = mapGetWithDefault(credential, this->apiPassphraseName);
            auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());

            // Construction du message à signer
            std::string preSign = timestamp + req.method_string().to_string() + req.target().to_string() + body;

            // Calcul de la signature avec HMAC SHA256
            auto sign = UtilAlgorithm::computeHash(UtilAlgorithm::ShaVersion::SHA256, preSign);
            auto signBase64 = UtilAlgorithm::base64Encode(sign);

            // Ajout des headers d'authentification
            req.set("OK-ACCESS-KEY", apiKey);
            req.set("OK-ACCESS-SIGN", signBase64);
            req.set("OK-ACCESS-TIMESTAMP", timestamp);
            req.set("OK-ACCESS-PASSPHRASE", apiPassphrase);

            if (!xSimulatedTrading.empty()) {
                req.set("x-simulated-trading", "1");
            }
        }

    private:
        std::string cancelBatchOrdersTarget;
        std::string apiPassphraseName;
        std::string apiXSimulatedTradingName;
    };
}

#endif
