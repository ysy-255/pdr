#include "pdr/pdr.hpp"

using namespace pdr;
using enum ParseError::Code;

std::expected<PDR, ParseError> PDR::parse(const std::string &filepath) {
	PDR pdr;
	CSV buffer(filepath);

	auto r1 = parseHeader(pdr, buffer);
	if (!r1) return std::unexpected(r1.error());

	auto r2 = parsePaths(pdr, buffer);
	if (!r2) return std::unexpected(r2.error());

	return pdr;
}

std::expected<void, ParseError> PDR::parseHeader(PDR& pdr, const CSV& buffer) {
	if (buffer.size() < 4) {
		return std::unexpected(ParseError{INVALID_FORMAT, 0, "Insufficient header lines"});
	}

	pdr.version = buffer[0][0];
	if (pdr.version != "PDR030") {
		return std::unexpected(ParseError{INVALID_FORMAT, 0, "Unsupported PDR version: " + pdr.version});
	}

	if (buffer[1].size() < 9) {
		return std::unexpected(ParseError{INVALID_FORMAT, 1, "Insufficient header fields"});
	}
	pdr.width = std::stoi(buffer[1][0]);
	pdr.height = std::stoi(buffer[1][1]);
	pdr.bg_color.r = static_cast<uint8_t>(std::stoi(buffer[1][2]));
	pdr.bg_color.g = static_cast<uint8_t>(std::stoi(buffer[1][3]));
	pdr.bg_color.b = static_cast<uint8_t>(std::stoi(buffer[1][4]));
	pdr.bg_color.a = 255; // 背景色は常に不透明とおく
	pdr.optimize_flag = std::stoi(buffer[1][5]);
	pdr.optimize_point = std::stoi(buffer[1][6]);
	pdr.optimize_line = std::stoi(buffer[1][7]);
	pdr.quality = std::stoi(buffer[1][8]);

	if (buffer[2].size() < 6) {
		return std::unexpected(ParseError{INVALID_FORMAT, 2, "Insufficient header fields"});
	}
	pdr.show_reference_image = std::stoi(buffer[2][0]);
	pdr.reference_image_path = buffer[2][1];
	pdr.reference_x = std::stoi(buffer[2][2]);
	pdr.reference_y = std::stoi(buffer[2][3]);
	pdr.reference_zoom = std::stof(buffer[2][4]);
	pdr.reference_opacity = static_cast<uint8_t>(std::stoi(buffer[2][5]) - 1);

	pdr.path_count = std::stoi(buffer[3][0]);
	return {};
}

std::expected<void, ParseError> PDR::parsePaths(PDR& pdr, const CSV& buffer) {
	int32_t line_idx = 4;
	for (int i = 0; i < pdr.path_count; i++) {
		if (line_idx >= buffer.size()) {
			return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path lines"});
		}
		auto path_r = parsePath(pdr, buffer, line_idx);
		if (!path_r) {
			return std::unexpected(path_r.error());
		}
		pdr.paths.push_back(path_r.value());
	}
	return {};
}

std::expected<Path, ParseError> PDR::parsePath(PDR& pdr, const CSV& buffer, int32_t& line_idx) {
	Path path;
	const auto& line = buffer[line_idx];
	if (line.size() < 17) return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path fields"});

	path.line_count = std::stoi(line[0]);
	auto line_closed_r = parseBool(line[1], line_idx);
	if (!line_closed_r) return std::unexpected(line_closed_r.error());
	path.closed_path = line_closed_r.value();
	path.line_width = std::stoi(line[2]);
	path.show_stroke = std::stoi(line[3]);
	path.stroke_color = parseRGBA(std::stoi(line[4]));
	path.fill_type = std::stoi(line[5]);
	if (path.fill_type < 0 || path.fill_type > 4) {
		return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid fill type"});
	}
	path.fill_color = parseRGBA(std::stoi(line[6]));
	path.grad_center_x = std::stof(line[7]);
	path.grad_center_y = std::stof(line[8]);
	path.grad_angle = std::stof(line[9]);
	path.grad_width = std::stof(line[10]);
	path.grad_height = std::stof(line[11]);
	path.grad_color = parseRGBA(std::stoi(line[12]));
	auto grouped_r = parseBool(line[13], line_idx);
	if (!grouped_r) return std::unexpected(grouped_r.error());
	path.grouped = grouped_r.value();
	path.group_num = std::stoi(line[14]);
	auto compound_path_r = parseBool(line[15], line_idx);
	if (!compound_path_r) return std::unexpected(compound_path_r.error());
	path.compound_path = compound_path_r.value();
	path.compound_num = std::stoi(line[16]);

	if (++line_idx >= buffer.size()) {
		return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path lines"});
	}
	path.gradient_control_point_count = std::stoi(buffer[line_idx][0]);

	if (line_idx + path.line_count + path.gradient_control_point_count >= buffer.size()) {
		return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient path lines"});
	}

	for (int i = 0; i < path.gradient_control_point_count; i++) {
		const auto& grad_line = buffer[++line_idx];
		if (grad_line.size() < 2) {
			return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient gradient control point fields"});
		}
		GradControlPoint gcp;
		gcp.position = static_cast<uint8_t>(std::stoi(grad_line[0]));
		gcp.color = parseRGBA(std::stoi(grad_line[1]));
		path.gradient_control_points.push_back(gcp);
	}

	for (int i = 0; i <= path.line_count; i++) {
		const auto& anchor_line = buffer[++line_idx];
		if (anchor_line.size() < 3) {
			return std::unexpected(ParseError{INVALID_FORMAT, line_idx, "Insufficient anchor fields"});
		}
		Anchor anchor;
		anchor.round_corner = std::stoi(anchor_line[0]);
		anchor.x = std::stoi(anchor_line[1]);
		anchor.y = std::stoi(anchor_line[2]);
		path.anchors.push_back(anchor);
	}

	if (!path.closed_path) {
		path.anchors.front().round_corner = false;
		path.anchors.back().round_corner = false;
	}

	line_idx++;

	return path;
}

std::expected<bool, ParseError> PDR::parseBool(const std::string& str, const int32_t& line_idx) {
	if (str == "#TRUE#") {
		return true;
	} else if (str == "#FALSE#") {
		return false;
	}
	return std::unexpected(ParseError{INVALID_VALUE, line_idx, "Invalid boolean value: " + str});
}

RGBA PDR::parseRGBA(const int32_t& color) {
	RGBA rgba;
	rgba.a = (color >> 24) & 0xFF;
	rgba.b = (color >> 16) & 0xFF;
	rgba.g = (color >> 8) & 0xFF;
	rgba.r = color & 0xFF;
	return rgba;
}
