cmake_minimum_required(VERSION 3.10)

project(cfgconv VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/include")

# add the executable
add_executable(cfgconv
	src/Instruction.cpp
	src/CfgNode.cpp
	src/CfgEdge.cpp
	src/CFG.cpp
	src/CFGReader.cpp
	src/InputTokenizer.cpp
	src/BFTraceReader.cpp
	src/DCFGReader.cpp
	src/cfgconv.cpp
)

target_include_directories(cfgconv PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           ${EXTRA_INCLUDES})
