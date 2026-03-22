#include "pdr/pdr.hpp"
#include <charconv>

using namespace pdr;
using enum ParseError::Code;

#define TRY(x) do { auto r = (x); if (!r) return std::unexpected(r.error()); } while(0)

std::expected<PDR, ParseError> PDR::parse(const std::string &filepath) {
	PDR pdr;
	CSV buffer(filepath);

	auto r_header = parseHeader(pdr, buffer);
	if (!r_header) return std::unexpected(r_header.error());

	auto r_paths = parsePaths(pdr, buffer);
	if (!r_paths) return std::unexpected(r_paths.error());

	return pdr;
}

std::expected<void, ParseError> PDR::parseHeader(PDR& pdr, const CSV& buffer) {
	if (buffer.size() < 4) {
		return std::unexpected(ParseError{INVALID_FORMAT, 0, "Insufficient header lines"});
	}

	pdr.version = buffer[0][0];
	if (pdr.version != "PDR030") {
		return std::unexpected(ParseError{INVALID_VALUE, 0, "Unsupported PDR version: " + pdr.version});
	}

	if (buffer[1].size() < 9) {
		return std::unexpected(ParseError{INVALID_FORMAT, 1, "Insufficient header fields"});
	}
	TRY(parseNum(buffer[1][0], pdr.width, 1));
	TRY(parseNum(buffer[1][1], pdr.height, 1));
	TRY(parseNum(buffer[1][2], pdr.bg_color.r, 1));
	TRY(parseNum(buffer[1][3], pdr.bg_color.g, 1));
	TRY(parseNum(buffer[1][4], pdr.bg_color.b, 1));
	pdr.bg_color.a = 255; // 背景色は常に不透明とおく
	TRY(parseNum(buffer[1][5], pdr.optimize_flag, 1)); // フラグは下位2ビットのみ用いるため範囲を無視する
	TRY(parseNum(buffer[1][6], pdr.optimize_point, 1));
	TRY(parseNum(buffer[1][7], pdr.optimize_line, 1));
	TRY(parseNum(buffer[1][8], pdr.quality, 1));
	if (pdr.quality < 0 || pdr.quality > 3) {
		return std::unexpected(ParseError{INVALID_VALUE, 1, "Invalid quality value"});
	}

	if (buffer[2].size() < 6) {
		return std::unexpected(ParseError{INVALID_FORMAT, 2, "Insufficient header fields"});
	}
	TRY(parseNum(buffer[2][0], pdr.show_reference_image, 2));
	pdr.reference_image_path = buffer[2][1];
	TRY(parseNum(buffer[2][2], pdr.reference_x, 2));
	TRY(parseNum(buffer[2][3], pdr.reference_y, 2));
	TRY(parseNum(buffer[2][4], pdr.reference_zoom, 2));
	int32_t reference_opacity_tmp;
	TRY(parseNum(buffer[2][5], reference_opacity_tmp, 2));
	if (reference_opacity_tmp < 1 || reference_opacity_tmp > 256) {
		return std::unexpected(ParseError{INVALID_VALUE, 2, "Invalid reference opacity value" + buffer[2][5]});
	}
	pdr.reference_opacity = static_cast<uint8_t>(reference_opacity_tmp - 1);
	return {};
}

std::expected<void, ParseError> PDR::parsePaths(PDR& pdr, const CSV& buffer) {
	size_t line_idx = 4;
	size_t path_count;
	TRY(parseNum(buffer[3][0], path_count, 3));
	for (size_t i = 0; i < path_count; i++) {
		if (line_idx >= buffer.size()) {
			return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path lines"});
		}
		auto path_r = parsePath(buffer, line_idx);
		if (!path_r) {
			return std::unexpected(path_r.error());
		}
		pdr.paths.push_back(path_r.value());
	}
	return {};
}

std::expected<Path, ParseError> PDR::parsePath(const CSV& buffer, size_t& line_idx) {
	Path path;
	const auto& line = buffer[line_idx];
	if (line.size() < 17) {
		return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path fields"});
	}

	size_t line_count;
	TRY(parseNum(line[0], line_count, line_idx));
	TRY(parseBOOL(line[1], path.closed_path, line_idx));
	TRY(parseNum(line[2], path.line_width, line_idx));
	TRY(parseNum(line[3], path.show_stroke, line_idx));
	TRY(parseRGBA(line[4], path.stroke_color, line_idx));
	TRY(parseNum(line[5], path.fill_type, line_idx));
	if (path.fill_type < 0 || path.fill_type > 4) {
		return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid fill type"});
	}
	TRY(parseRGBA(line[6], path.fill_color, line_idx));
	TRY(parseNum(line[7], path.grad_center_x, line_idx));
	TRY(parseNum(line[8], path.grad_center_y, line_idx));
	TRY(parseNum(line[9], path.grad_angle, line_idx));
	TRY(parseNum(line[10], path.grad_width, line_idx));
	TRY(parseNum(line[11], path.grad_height, line_idx));
	TRY(parseRGBA(line[12], path.grad_color, line_idx));
	TRY(parseBOOL(line[13], path.grouped, line_idx));
	TRY(parseNum(line[14], path.group_num, line_idx));
	TRY(parseBOOL(line[15], path.compound_path, line_idx));
	TRY(parseNum(line[16], path.compound_num, line_idx));

	if (++line_idx >= buffer.size()) {
		return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path lines"});
	}
	
	size_t gradient_control_point_count;
	TRY(parseNum(buffer[line_idx][0], gradient_control_point_count, line_idx));

	if (line_idx + line_count + gradient_control_point_count >= buffer.size()) {
		return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path lines"});
	}

	for (size_t i = 0; i < gradient_control_point_count; i++) {
		const auto& grad_line = buffer[++line_idx];
		if (grad_line.size() < 2) {
			return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient gradient control point fields"});
		}
		GradControlPoint gcp;
		TRY(parseNum(grad_line[0], gcp.position, line_idx));
		if (gcp.position < 1 || gcp.position > 254) {
			return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid gradient control point position"});
		}
		TRY(parseRGBA(grad_line[1], gcp.color, line_idx));
		path.gradient_control_points.push_back(gcp);
	}

	for (size_t i = 0; i <= line_count; i++) {
		const auto& anchor_line = buffer[++line_idx];
		if (anchor_line.size() < 3) {
			return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient anchor fields"});
		}
		Anchor anchor;
		TRY(parseNum(anchor_line[0], anchor.round_corner, line_idx));
		TRY(parseNum(anchor_line[1], anchor.x, line_idx));
		TRY(parseNum(anchor_line[2], anchor.y, line_idx));
		path.anchors.push_back(anchor);
	}

	if (!path.closed_path) {
		path.anchors.front().round_corner = false;
		path.anchors.back().round_corner = false;
	}

	line_idx++;

	return path;
}

std::expected<void, ParseError> PDR::parseBOOL(const std::string& str, bool& result, const size_t& line_idx) {
	if (str == "#TRUE#") {
		result = true;
		return {};
	} else if (str == "#FALSE#") {
		result = false;
		return {};
	}
	return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid boolean value: " + str});
}

template<typename T>
std::expected<void, ParseError> PDR::parseNum(const std::string& str, T& result, const size_t& line_idx) {
	auto res = std::from_chars(str.data(), str.data() + str.size(), result);
	if (res.ec != std::errc()) {
		return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid numeric value: " + str});
	}
	return {};
}

std::expected<void, ParseError> PDR::parseNum(const std::string& str, bool& result, const size_t& line_idx) {
	if (str == "0") {
		result = false;
		return {};
	} else if (str == "1") {
		result = true;
		return {};
	}
	return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid boolean numeric value: " + str});
}

std::expected<void, ParseError> PDR::parseRGBA(const std::string& str, RGBA& result, const size_t& line_idx) {
	int32_t color;
	TRY(parseNum(str, color, line_idx));
	result = parseRGBA(color);
	return {};
}

RGBA PDR::parseRGBA(const int32_t& color) {
	RGBA rgba;
	rgba.a = (color >> 24) & 0xFF;
	rgba.b = (color >> 16) & 0xFF;
	rgba.g = (color >> 8) & 0xFF;
	rgba.r = color & 0xFF;
	return rgba;
}

#undef TRY
