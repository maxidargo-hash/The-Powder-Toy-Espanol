#pragma once

#include <array>
#include <cstddef>

// The original mobile font uses one byte per glyph. These slots were unused.
template<std::size_t N>
constexpr std::array<char, N> EncodeSpanishText(const char (&text)[N])
{
	constexpr char characters[] = "áéíóúüñÁÉÍÓÚÜÑ¿¡";
	constexpr unsigned char glyphs[] = {
		0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
		0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x89,
	};
	std::array<char, N> encoded{};
	std::size_t out = 0;
	for (std::size_t pos = 0; pos + 1 < N; ++pos)
	{
		if ((static_cast<unsigned char>(text[pos]) == 0xC2 ||
		     static_cast<unsigned char>(text[pos]) == 0xC3) && pos + 2 < N)
		{
			bool found = false;
			for (std::size_t i = 0; i < sizeof(glyphs); ++i)
			{
				if (text[pos] == characters[2 * i] && text[pos + 1] == characters[2 * i + 1])
				{
					encoded[out++] = static_cast<char>(glyphs[i]);
					++pos;
					found = true;
					break;
				}
			}
			if (found)
				continue;
		}
		encoded[out++] = text[pos];
	}
	return encoded;
}

// Keep the source strings in UTF-8 and the storage alive for legacy controls.
#define TPT_ES(text) ([] { static constexpr auto label = EncodeSpanishText(text); return label.data(); }())
