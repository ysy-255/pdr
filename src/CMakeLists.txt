add_library(pdr
	pdr.cpp
	pdr2svg.cpp
)

target_include_directories(pdr
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(pdr PRIVATE -Wall -Wextra)
