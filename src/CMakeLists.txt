add_library(pdr
	pdr.cpp
	svg_decoder.cpp
)

target_include_directories(pdr
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(pdr PRIVATE -Wall -Wextra)
