add_executable(pdr2svg
	main.cpp
)

target_link_libraries(pdr2svg PRIVATE pdr)
target_compile_options(pdr2svg PRIVATE -Wall -Wextra)
