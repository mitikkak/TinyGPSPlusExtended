project(ut)

set(STUBS_DIR ${PROJECT_ROOT}/arduino_cmake/ut/stubs)
set(TESTS_DIR ${PROJECT_ROOT}/tests)
set(sources
    ${SRC_DIR}/TinyGPS++.cpp
)
set(stub_sources
    ${STUBS_DIR}/Arduino.cpp
    ${STUBS_DIR}/PinEvents.cpp
)
set(test_sources
    ${TESTS_DIR}/TestTinyGpsPlus.cpp
    ${TESTS_DIR}/TestChecksums.cpp
    # Keep this last
    ${TESTS_DIR}/Main.cpp
)

add_executable(${PROJECT_NAME} ${test_sources} ${sources} ${stub_sources})

set(GTEST_LIBRARIES gtest gmock)
target_link_libraries(${PROJECT_NAME} ${GTEST_LIBRARIES} pthread)

target_compile_options(
    ${PROJECT_NAME} PRIVATE
    -std=gnu++11
)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
      ${PROJECT_ROOT}/googletest/googletest/include
      ${SRC_DIR}
      ${STUBS_DIR}
)
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    ARDUINO=10800
)
