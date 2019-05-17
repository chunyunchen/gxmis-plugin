project(exchange)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( exchange exchange exchange.cpp )
target_include_directories( exchange PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( exchange ${CMAKE_SOURCE_DIR}/../ricardian )