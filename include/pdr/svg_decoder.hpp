#pragma once

#include "pdr/pdr.hpp"

using namespace pdr;

struct SVGDecoder {
public:
	SVGDecoder();
	bool decodeSVG(PDR& pdr_file, const std::string& svg_filepath);

private:
	std::string generateSVGHeader(const PDR& pdr_file);
	std::string generateGradient(const Path& path, int path_index);
	std::string generatePathElement(const Path& path, int path_index);
	std::string generatePathD(const Path& path);
};
