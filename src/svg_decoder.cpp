#include "pdr/svg_decoder.hpp"
#include <fstream>
#include <iomanip>
#include <cmath>

SVGDecoder::SVGDecoder() {}

bool SVGDecoder::decodeSVG(PDR& pdr_file, const std::string& svg_filepath) {
	std::ostringstream oss;
	oss << generateSVGHeader(pdr_file);
	oss << "\t<rect width=\"" << pdr_file.width << "\" height=\"" << pdr_file.height << "\" fill=\"" << pdr_file.bg_color.hex() << "\" />\n";
	oss << "\t<defs>\n";
	for (int i = 0; i < pdr_file.paths.size(); ++i) {
		const Path& path = pdr_file.paths[i];
		if (path.fill_type > 1) {
			oss << generateGradient(path, i);
		}
	}
	oss << "\t</defs>\n";
	for (int i = 0; i < pdr_file.paths.size(); ++i) {
		oss << generatePathElement(pdr_file.paths[i], i);
	}
	oss << "</svg>\n";
	std::ofstream file(svg_filepath);
	if (!file.is_open()) return false;
	file << oss.str();

	return true;
}

std::string SVGDecoder::generateSVGHeader(const PDR& pdr_file) {
	std::ostringstream oss;
	oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	oss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
	oss << "width=\"" << pdr_file.width << "\" ";
	oss << "height=\"" << pdr_file.height << "\" ";
	oss << "viewBox=\"0 0 " << pdr_file.width << " " << pdr_file.height << "\">\n";
	return oss.str();
}

std::string SVGDecoder::generateGradient(const Path& path, int path_index) {
	std::ostringstream oss;
	if (path.fill_type < 4) {
		oss << "\t\t<linearGradient ";
	} else {
		oss << "\t\t<radialGradient ";
	}
	oss << "id=\"gradient_" << path_index << "\" ";
	oss << "gradientUnits=\"userSpaceOnUse\" ";

	if (path.fill_type < 4) {
		oss << "x1=\"" << (path.grad_center_x - path.grad_width * 16384) / 20 << "\" ";
		oss << "x2=\"" << (path.grad_center_x + path.grad_width * 16384) / 20 << "\" ";
		oss << "gradientTransform=\"rotate(" << path.grad_angle << " " << path.grad_center_x / 20 << " " << path.grad_center_y / 20 << ")\"";
	} else {
		float width = path.grad_width * 16384 / 20;
		float height = path.grad_height * 16384 / 20;
		oss << "cx=\"0\" ";
		oss << "cy=\"0\" ";
		oss << "r=\"1\" ";
		oss << "gradientTransform=\"scale(" << width << " " << height << ") "
			<< "translate(" << path.grad_center_x / 20 / width << " " << path.grad_center_y / 20 / height << ") "
			<< "rotate(" << path.grad_angle << ")\"";
	}

	oss << ">\n";
	if (path.fill_type == 2) {
		oss << "\t\t\t<stop offset=\"0\" stop-color=\"" << path.fill_color.hex() << "\" />\n";
		for (int i = 0; i < path.gradient_control_point_count; ++i) {
			const auto& cp = path.gradient_control_points[i];
			oss << "\t\t\t<stop offset=\"" << std::fixed << std::setprecision(3) << cp.position / 256.0 << "\" stop-color=\"" << cp.color.hex() << "\" />\n";
		}
		oss << "\t\t\t<stop offset=\"1\" stop-color=\"" << path.grad_color.hex() << "\" />\n";
	} else if (path.fill_type == 3) {
		oss << "\t\t\t<stop offset=\"0\" stop-color=\"" << path.fill_color.hex() << "\" />\n";
		for (int i = 0; i < path.gradient_control_point_count; ++i) {
			const auto& cp = path.gradient_control_points[i];
			oss << "\t\t\t<stop offset=\"" << std::fixed << std::setprecision(3) << cp.position / 256.0 / 2 << "\" stop-color=\"" << cp.color.hex() << "\" />\n";
		}
		oss << "\t\t\t<stop offset=\"0.5\" stop-color=\"" << path.grad_color.hex() << "\" />\n";
		for (int i = 0; i-- > 0;) {
			const auto& cp = path.gradient_control_points[i];
			oss << "\t\t\t<stop offset=\"" << std::fixed << std::setprecision(3) << 1 - cp.position / 256.0 / 2 << "\" stop-color=\"" << cp.color.hex() << "\" />\n";
		}
		oss << "\t\t\t<stop offset=\"1\" stop-color=\"" << path.fill_color.hex() << "\" />\n";
	} else if (path.fill_type == 4) {
		oss << "\t\t\t<stop offset=\"0\" stop-color=\"" << path.fill_color.hex() << "\" />\n";
		for (int i = 0; i < path.gradient_control_point_count; ++i) {
			const auto& cp = path.gradient_control_points[i];
			oss << "\t\t\t<stop offset=\"" << std::fixed << std::setprecision(3) << cp.position / 256.0 << "\" stop-color=\"" << cp.color.hex() << "\" />\n";
		}
		oss << "\t\t\t<stop offset=\"1\" stop-color=\"" << path.grad_color.hex() << "\" />\n";
	}
	if (path.fill_type < 4) {
		oss << "\t\t</linearGradient>\n";
	} else {
		oss << "\t\t</radialGradient>\n";
	}
	return oss.str();
}

std::string SVGDecoder::generatePathElement(const Path& path, int path_index) {
	std::ostringstream oss;
	oss << "\t<path d=\"" << generatePathD(path) << "\" ";
	if (path.show_stroke || path.fill_type == 0 || !path.closed_path) {
		oss << "stroke=\"" << path.stroke_color.hex() << "\" ";
		oss << "stroke-width=\"" << (path.line_width / 20.0) << "\" ";
	} else {
		oss << "stroke=\"none\" ";
	}
	if (path.fill_type == 0 || !path.closed_path) {
		oss << "fill=\"none\"";
	} else if (path.fill_type == 1) {
		oss << "fill=\"" << path.fill_color.hex() << "\"";
	} else if (path.fill_type > 1) {
		oss << "fill=\"url(#gradient_" << path_index << ")\"";
	}
	oss << "/>\n";
	return oss.str();
}

std::string SVGDecoder::generatePathD(const Path& path) {
	std::ostringstream oss;
	if (path.anchors.empty()) return "";
	const auto& first_anchor = path.anchors[0];
	if (path.anchors.size() == 1) {
		oss << "M " << (first_anchor.x / 20) << " " << (first_anchor.y / 20);
		oss << "L " << (first_anchor.x / 20) << " " << (first_anchor.y / 20);
		return oss.str();
	}
	const auto& second_anchor = path.anchors[1];
	bool lround;
	if (path.closed_path && first_anchor.round_corner) {
		if (second_anchor.round_corner) {
			oss << "M " << (first_anchor.x + second_anchor.x) / 40 << " " << (first_anchor.y + second_anchor.y) / 40 << " ";
			lround = false;
		} else {
			oss << "M " << (second_anchor.x / 20) << " " << (second_anchor.y / 20) << " ";
			lround = true;
		}
	} else {
		oss << "M " << (first_anchor.x / 20) << " " << (first_anchor.y / 20) << " ";
		lround = false;
	}
	for (int i = 1; i <= path.line_count; ++i) {
		const auto& anchor = path.anchors[i];
		if (anchor.round_corner) {
			if (lround) {
				oss << "T ";
			} else {
				oss << "Q " << (anchor.x / 20) << " " << (anchor.y / 20) << " ";
			}
			const auto& next_anchor = path.anchors[i < path.line_count ? i + 1 : 0];
			if (next_anchor.round_corner) {
				oss << ((anchor.x + next_anchor.x) / 40) << " " << ((anchor.y + next_anchor.y) / 40) << " ";
			} else {
				oss << (next_anchor.x / 20) << " " << (next_anchor.y / 20) << " ";
			}
			lround = true;
		} else {
			if (!lround) {
				oss << "L " << (anchor.x / 20) << " " << (anchor.y / 20) << " ";
			}
			lround = false;
		}
	}
	if (path.closed_path) {
		if (lround) {
			if (first_anchor.round_corner) {
				if (second_anchor.round_corner) {
					oss << "T " << ((first_anchor.x + second_anchor.x)) / 40 << " " << ((first_anchor.y + second_anchor.y)) / 40 << "Z";
				} else {
					oss << "T " << (second_anchor.x / 20) << " " << (second_anchor.y / 20) << "Z";
				}
			} else {
				oss << "Z";
			}
		} else {
			if (first_anchor.round_corner) {
				if (second_anchor.round_corner) {
					oss << "Q " << (first_anchor.x / 20) << " " << (first_anchor.y / 20) << " " << ((first_anchor.x + second_anchor.x) / 40) << " " << ((first_anchor.y + second_anchor.y) / 40) << "Z";
				} else {
					oss << "Q " << (first_anchor.x / 20) << " " << (first_anchor.y / 20) << " " << (second_anchor.x / 20) << " " << (second_anchor.y / 20) << "Z";
				}
			} else {
				oss << "L " << (first_anchor.x / 20) << " " << (first_anchor.y / 20) << "Z";
			}
		}
	}
	return oss.str();
}
