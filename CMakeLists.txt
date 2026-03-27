cmake_minimum_required(VERSION 3.16)
project(biogears_digital_twin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  cpp_httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(cpp_httplib)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(biogears_digital_twin
  src/main.cpp
  src/BioGearsEngineManager.cpp
  src/EventProcessor.cpp
  src/PhysiologyState.cpp
  src/SimulationLoop.cpp
  src/ApiServer.cpp
)

target_include_directories(biogears_digital_twin
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /opt/biogears/usr/include
)

target_link_directories(biogears_digital_twin PRIVATE /opt/biogears/usr/lib)

target_link_libraries(biogears_digital_twin
  PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    biogears
    biogears_io
    biogears_cdm
    pthread
)

set_target_properties(biogears_digital_twin PROPERTIES
  BUILD_RPATH "/opt/biogears/usr/lib"
  INSTALL_RPATH "/opt/biogears/usr/lib"
)
