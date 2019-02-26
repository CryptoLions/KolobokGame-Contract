project(KolobokGame)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( KolobokGame KolobokGame KolobokGame.cpp )
target_include_directories( KolobokGame PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( KolobokGame ${CMAKE_SOURCE_DIR}/../ricardian )