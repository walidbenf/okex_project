cmake_minimum_required(VERSION 3.14)
project(okex_connector CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chemin vers la bibliothèque CCAPI
set(CCAPI_ROOT_DIR "/usr/local/include" CACHE PATH "Chemin vers CCAPI")

# Options
option(CCAPI_ENABLE_EXCHANGE_OKEX "Enable OKEX exchange" ON)
option(CCAPI_ENABLE_SERVICE_EXECUTION_MANAGEMENT "Enable execution management service" ON)
option(CCAPI_ENABLE_SERVICE_MARKET_DATA "Enable market data service" ON)

# Ajout des définitions
if(CCAPI_ENABLE_EXCHANGE_OKEX)
    add_definitions(-DCCAPI_ENABLE_EXCHANGE_OKEX)
endif()
if(CCAPI_ENABLE_SERVICE_EXECUTION_MANAGEMENT)
    add_definitions(-DCCAPI_ENABLE_SERVICE_EXECUTION_MANAGEMENT)
endif()
if(CCAPI_ENABLE_SERVICE_MARKET_DATA)
    add_definitions(-DCCAPI_ENABLE_SERVICE_MARKET_DATA)
endif()

# Trouver les dépendances
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

# Définir les fichiers sources
set(SOURCES
    src/ccapi_market_data_service_okex.cpp
)

# Inclure les répertoires
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/ccapi_okex
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Ajouter la bibliothèque
add_library(ccapi_okex ${SOURCES})

# Définir explicitement comme une bibliothèque C++
set_target_properties(ccapi_okex PROPERTIES
    LINKER_LANGUAGE CXX
)

# Lier les dépendances
target_link_libraries(ccapi_okex
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)

# Ajouter les tests
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_testing()
    add_executable(test_okex test/test_execution_management_service_okex.cpp)
    add_executable(test_okex_integration test/test_okex_integration.cpp)

    target_link_libraries(test_okex
        ccapi_okex
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        pthread
    )

    target_link_libraries(test_okex_integration
        ccapi_okex
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        pthread
    )
endif()
