#pragma once

#include "csv.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <expected>

namespace pdr{

struct RGBA{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	std::string hex() const {
		char buf[10];
		snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", r, g, b, a);
		return std::string(buf);
	}
};

struct Anchor{
	bool round_corner;
	int32_t x; // px * 20
	int32_t y; // px * 20
};

struct GradControlPoint{
	uint8_t position;
	RGBA color;
};

struct Path{
	int32_t line_count;
	bool closed_path;
	int32_t line_width; // px * 20
	bool show_stroke; // fill_type=0 のときは値に関わらず線を表示
	RGBA stroke_color;
	int32_t fill_type; // 0=塗り無し, 1=ベタ塗り, 2=線形グラデ1, 3=線形グラデ2, 4=円形グラデ
	RGBA fill_color;
	float grad_center_x; // px * 20
	float grad_center_y; // px * 20
	float grad_angle; // 度数法
	float grad_width; // px * 20 / 32768
	float grad_height; // px * 20 / 32768
	RGBA grad_color;
	bool grouped;
	int32_t group_num;
	bool compound_path;
	int32_t compound_num;

	int32_t gradient_control_point_count;
	std::vector<Anchor> anchors;
	std::vector<GradControlPoint> gradient_control_points;
};

struct ParseError{
	enum class Code{
		FILE_NOT_FOUND,
		INVALID_FORMAT,
		INVALID_VALUE
	};
	Code code;
	int32_t line = 0;
	std::string message;
};

struct PDR{
	std::string version;
	int32_t width; // px
	int32_t height; // px
	RGBA bg_color;
	int32_t optimize_flag;
	int32_t optimize_point;
	int32_t optimize_line;
	int32_t quality;

	bool show_reference_image;
	std::string reference_image_path;
	int32_t reference_x; // px
	int32_t reference_y; // px
	float reference_zoom;
	uint8_t reference_opacity;

	int32_t path_count;
	std::vector<Path> paths;

	static std::expected<PDR, ParseError> parse(const std::string& filepath);

private:
	static std::expected<void, ParseError> parseHeader(PDR& pdr, const CSV& buffer);
	static std::expected<void, ParseError> parsePaths(PDR& pdr, const CSV& buffer);
	static std::expected<Path, ParseError> parsePath(PDR& pdr, const CSV& buffer, int32_t& line_idx);

	static std::expected<bool, ParseError> parseBool(const std::string& str, const int32_t& line_idx);
	static RGBA parseRGBA(const int32_t& color);
};

} // namespace pdr
