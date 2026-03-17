#include "pdr/pdr.hpp"
#include "pdr/pdr2svg.hpp"
#include <iostream>

using namespace pdr;

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Usage: pdr2svg <input.pdr> <output.svg>" << std::endl;
		return 1;
	}

	const std::string input_filepath = argv[1];
	const std::string output_filepath = argv[2];

	auto parse_r = PDR::parse(input_filepath);
	if (!parse_r) {
		std::cerr << "line " << parse_r.error().line + 1 << ": " << parse_r.error().message << std::endl;
		return 1;
	}
	PDR pdr = std::move(parse_r.value());

	const std::string svg_content = pdr2svg::render(pdr);
	std::ofstream file(output_filepath);
	if (!file.is_open()) {
		std::cerr << "Failed to open output file: " << output_filepath << std::endl;
		return 1;
	}
	file << svg_content;

	std::cout << "Successfully exported SVG file: " << output_filepath << std::endl;
	return 0;
}
