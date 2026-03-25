#include "pdr/pdr2svg.hpp"
#include <fstream>
#include <iomanip>
#include <cmath>

namespace pdr2svg {

// 整数を20で割って最短の文字列表現で返す
std::string div20_str(const int32_t num) {
	std::string res;
	int32_t q1 = num / 20;
	int32_t r1 = std::abs(num % 20);
	res = std::to_string(q1);
	if (r1 != 0) {
		int32_t q2 = r1 / 2;
		int32_t r2 = r1 % 2;
		res += '.';
		res += '0' + q2;
		if (r2 == 1) {
			res += '5';
		}
	}
	return res;
}
std::string div20_str(const double& num) {
	return div20_str(static_cast<int32_t>(std::round(num)));
}

std::string div100_str(const int32_t num) {
	std::string res;
	int32_t q1 = num / 100;
	int32_t r1 = std::abs(num % 100);
	res = std::to_string(q1);
	if (r1 != 0) {
		int32_t q2 = r1 / 10;
		int32_t r2 = r1 % 10;
		res += '.';
		res += '0' + q2;
		if (r2 != 0) {
			res += '0' + r2;
		}
	}
	return res;
}
std::string div100_str(const double& num) {
	return div100_str(static_cast<int32_t>(std::round(num)));
}

// 小数点以下2桁、不要な0を除く
std::string decimal2(const double& num) {
	return div100_str(num * 100);
}

std::string div1000_str(const int32_t num) {
	std::string res;
	int32_t q1 = num / 1000;
	int32_t r1 = std::abs(num % 1000);
	res = std::to_string(q1);
	if (r1 != 0) {
		int32_t q2 = r1 / 100;
		int32_t r2 = r1 % 100;
		res += '.';
		res += '0' + q2;
		if (r2 != 0) {
			int32_t q3 = r2 / 10;
			int32_t r3 = r2 % 10;
			res += '0' + q3;
			if (r3 != 0) {
				res += '0' + r3;
			}
		}
	}
	return res;
}
std::string div1000_str(const double& num) {
	return div1000_str(static_cast<int32_t>(std::round(num)));
}

// 小数点以下3桁、不要な0を除く
std::string decimal3(const double& num) {
	return div1000_str(num * 1000);
}

std::string generate_header(const PDR& pdr) {
	std::ostringstream oss;
	oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	oss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
	oss << "width=\"" << pdr.width << "\" ";
	oss << "height=\"" << pdr.height << "\" ";
	oss << "viewBox=\"0 0 " << pdr.width << ' ' << pdr.height << "\">\n";
	return oss.str();
}

std::string generate_gradient(const Path& path, size_t path_index) {
	std::ostringstream oss;
	const bool linear = path.fill_type < 4;
	const bool mirror = path.fill_type == 3;

	const float gwidth = path.grad_width * 16384;
	const float gheight = path.grad_height * 16384;
	const int32_t& gx = path.grad_center_x;
	const int32_t& gy = path.grad_center_y;
	const float& gangle = path.grad_angle;

	oss << "\t\t<" << (linear ? "linear" : "radial") << "Gradient ";
	oss << "id=\"gradient_" << path_index << "\" ";
	oss << "gradientUnits=\"userSpaceOnUse\" ";
	if (linear) {
		oss << "x1=\"" << div20_str(gx - gwidth) << "\" ";
		oss << "x2=\"" << div20_str(gx + gheight) << "\" ";
		oss << "gradientTransform=\""
			<< "rotate(" << decimal2(gangle) << ' ' << div20_str(gx) << ' ' << div20_str(gy) << ")\"";
	} else {
		oss << "cx=\"0\" ";
		oss << "cy=\"0\" ";
		oss << "r=\"1\" ";
		oss << "gradientTransform=\""
			<< "translate(" << div20_str(gx) << ' ' << div20_str(gy) << ") "
			<< "scale(" << div20_str(gwidth) << ' ' << div20_str(gheight) << ") "
			<< "rotate(" << decimal2(gangle) << ")\"";
	}
	oss << ">\n";

	struct GCP{
		float offset; // 0~1
		RGBA color;
	};

	size_t gcp_count = path.gradient_control_point_count();
	size_t gcp_arr_size = gcp_count + 2;
	if (mirror) gcp_arr_size = gcp_arr_size * 2 - 1;
	std::vector<GCP> gcp_arr(gcp_arr_size);
	gcp_arr.front().offset = 0;
	gcp_arr.front().color = path.fill_color;
	gcp_arr.back().offset = 1;
	gcp_arr.back().color = (mirror ? path.fill_color : path.grad_color);
	if (mirror) {
		gcp_arr[gcp_count + 1].offset = 0.5;
		gcp_arr[gcp_count + 1].color = path.grad_color;
	}
	const int32_t position_divider = (mirror ? 512 : 256);
	for (size_t i = 0; i < gcp_count; ++i) {
		const auto & gcp = path.gradient_control_points[i];
		auto & target = gcp_arr[i + 1];
		target.offset = gcp.position;
		target.offset /= position_divider;
		target.color = gcp.color;
		if (mirror) {
			target = gcp_arr[gcp_arr_size - 2 - i];
			target.offset = 512 - gcp.position;
			target.offset /= position_divider;
			target.color = gcp.color;
		}
	}

	for (const auto & gcp : gcp_arr) {
		oss << "\t\t\t<stop ";
		oss << "offset=\"" << decimal3(gcp.offset) << "\" ";
		oss << "stop-color=\"#" << gcp.color.rgbhex() << "\" ";
		if (gcp.color.a != 0xFF) {
			oss << "stop-opacity=\"" << decimal3(double(gcp.color.a) / 255) << "\" ";
		}
		oss << "/>\n";
	}

	oss << "\t\t</" << (linear ? "linear" : "radial") << "Gradient>\n";

	return oss.str();
}

std::string generate_path_d(const Path& path) {
	std::ostringstream oss;
	if (path.anchors.empty()) return "";
	const auto& first = path.anchors[0];
	if (path.anchors.size() == 1) {
		oss << 'M' << div20_str(first.x) << ' ' << div20_str(first.y)
			<< 'L' << div20_str(first.x) << ' ' << div20_str(first.y) << 'Z';
		return oss.str();
	}
	const auto& second = path.anchors[1];
	bool lround;
	if (path.closed_path && first.round_corner) {
		if (second.round_corner) {
			oss << 'M' << div20_str((first.x + second.x) / 2) << ' ' << div20_str((first.y + second.y) / 2);
			lround = false;
		} else {
			oss << 'M' << div20_str(second.x) << ' ' << div20_str(second.y);
			lround = true;
		}
	} else {
		oss << 'M' << div20_str(first.x) << ' ' << div20_str(first.y);
		lround = false;
	}
	const size_t& line_cnt = path.line_count();
	size_t anchor_cnt = line_cnt + 1 + (path.closed_path ? 1 : 0);
	for (size_t i = 1; i < anchor_cnt; ++i) {
		const auto& present = path.anchors[i <= line_cnt ? i : i - line_cnt - 1];
		if (present.round_corner) {
			if (lround) {
				oss << 'T';
			} else {
				oss << 'Q' << div20_str(present.x) << ' ' << div20_str(present.y) << ' ';
			}
			const auto& next = path.anchors[i < line_cnt ? i + 1 : i - line_cnt];
			if (next.round_corner) {
				oss << div20_str((present.x + next.x) / 2) << ' ' << div20_str((present.y + next.y) / 2);
			} else {
				oss << div20_str(next.x) << ' ' << div20_str(next.y);
			}
			lround = true;
		} else {
			if (!lround) {
				oss << 'L' << div20_str(present.x) << ' ' << div20_str(present.y);
			}
			lround = false;
		}
	}
	if (path.closed_path) {
		oss << 'Z';
	}
	return oss.str();
}

std::string generate_path(const Path& path, int path_index) {
	std::ostringstream oss;
	oss << "\t<path ";
	if (path.fill_type == 0 || !path.closed_path) {
		oss << "fill=\"none\" ";
	} else if (path.fill_type == 1) {
		oss << "fill=\"#" << path.fill_color.rgbhex() << "\" ";
		if (path.fill_color.a != 255) {
			oss << "fill-opacity=\"" << decimal3(double(path.fill_color.a) / 255) << "\" ";
		}
	} else if (path.fill_type > 1) {
		oss << "fill=\"url(#gradient_" << path_index << ")\" ";
	}
	if (path.show_stroke || path.fill_type == 0 || !path.closed_path) {
		oss << "stroke=\"#" << path.stroke_color.rgbhex() << "\" ";
		if (path.stroke_color.a != 255) {
			oss << "stroke-opacity=\"" << decimal3(double(path.stroke_color.a) / 255) << "\" ";
		}
		oss << "stroke-width=\"" << div20_str(path.line_width) << "\" ";
	}
	oss << "d=\"" << generate_path_d(path) << "\" ";
	oss << "/>\n";
	return oss.str();
}

std::string render(const pdr::PDR& pdr) {
	std::ostringstream oss;
	oss << generate_header(pdr);
	oss << "\t<rect ";
	oss << "width=\"" << pdr.width << "\" ";
	oss << "height=\"" << pdr.height << "\" ";
	oss << "fill=\"#" << pdr.bg_color.rgbhex() << "\" ";
	oss << "/>\n";

	oss << "\t<defs>\n";
	for (size_t i = 0; i < pdr.paths.size(); ++i) {
		const Path& path = pdr.paths[i];
		if (path.closed_path && path.fill_type > 1) {
			oss << generate_gradient(path, i);
		}
	}
	oss << "\t</defs>\n";
	for (size_t i = 0; i < pdr.paths.size(); ++i) {
		oss << generate_path(pdr.paths[i], i);
	}
	oss << "</svg>\n";
	return oss.str();
}

}
