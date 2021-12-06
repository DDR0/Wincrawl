#pragma once

///Teach Visual Studio how to hash values smaller than the hash output, by not.
struct LiteralHash {
	template <typename T>
	inline std::size_t operator()(T t) const {
		return static_cast<std::size_t>(t);
	}
};