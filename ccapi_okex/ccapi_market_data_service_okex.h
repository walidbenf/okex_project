/**
 * @file ccapi_market_data_service_okex.h
 * @brief Service de données de marché pour OKX
 *
 * Implémente les fonctionnalités de streaming et de requêtes REST
 * pour obtenir les données de marché d'OKX (prix, trades, orderbook, etc.)
 */

#ifndef CCAPI_MARKET_DATA_SERVICE_OKEX_H
#define CCAPI_MARKET_DATA_SERVICE_OKEX_H

#include "../ccapi_cpp/service/ccapi_market_data_service.h"

namespace ccapi {
    /**
     * @brief Service de données de marché OKX
     *
     * Fournit l'accès aux données de marché via :
     * - WebSocket pour les mises à jour en temps réel
     * - REST API pour les requêtes ponctuelles
     */
    class MarketDataServiceOkex : public MarketDataService {
        public:
            /**
             * @brief Constructeur du service
             *
             * @param eventHandler Handler pour traiter les événements reçus
             * @param sessionOptions Options de configuration de la session
             * @param sessionConfigs Configuration de la session
             * @param serviceContextPtr Contexte du service
             */
            MarketDataServiceOkex(std::function<void(Event&, Queue<Event>*)> eventHandler,
                                SessionOptions sessionOptions,
                                SessionConfigs sessionConfigs,
                                ServiceContext* serviceContextPtr);
            virtual ~MarketDataServiceOkex();

        protected:
            /**
             * @brief Convertit une requête en format WebSocket
             *
             * Transforme les paramètres de subscription en message WebSocket
             * selon le format attendu par OKX.
             *
             * @param subscription Paramètres de subscription
             * @param now Timestamp actuel
             * @param symbolId Identifiant du symbole
             * @return Message WebSocket formaté
             */
            std::string convertSubscriptionToWebsocketMessage(const Subscription& subscription,
                                                            const TimePoint& now,
                                                            const std::string& symbolId);

            /**
             * @brief Traite les messages WebSocket reçus
             *
             * Parse et traite les messages de l'API WebSocket OKX.
             * Convertit les données en événements CCAPI.
             *
             * @param wsConnection Connexion WebSocket
             * @param textMessage Message reçu
             * @param timeReceived Timestamp de réception
             */
            void onTextMessage(WsConnection& wsConnection,
                             const Subscription& subscription,
                             const std::string& textMessage,
                             const TimePoint& timeReceived);
    };
}

#endif
