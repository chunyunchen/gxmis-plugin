file(GLOB HEADERS "include/eosio/data_exchange_plugin/*.hpp")
add_library( data_exchange_plugin
             data_exchange_plugin.cpp
             httpc.cpp
             ${HEADERS} )

target_link_libraries( data_exchange_plugin http_plugin chain_plugin eosio_chain appbase fc )
target_include_directories( data_exchange_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include")
