#pragma once

struct LiteralHash {
	template <typename T>
	inline std::size_t operator()(T t) const {
		return static_cast<std::size_t>(t);
	}
};