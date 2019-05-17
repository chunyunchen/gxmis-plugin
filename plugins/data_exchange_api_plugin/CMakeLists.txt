file(GLOB HEADERS "include/eosio/data_exchange_api_plugin/*.hpp")
add_library( data_exchange_api_plugin
             data_exchange_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( data_exchange_api_plugin data_exchange_plugin http_plugin appbase fc )
target_include_directories( data_exchange_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
