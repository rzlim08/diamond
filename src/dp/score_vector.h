/****
DIAMOND protein aligner
Copyright (C) 2013-2021 Max Planck Society for the Advancement of Science e.V.
                        Benjamin Buchfink
                        Eberhard Karls Universitaet Tuebingen
						
Code developed by Benjamin Buchfink <benjamin.buchfink@tue.mpg.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#pragma once
#include <algorithm>
#include <limits.h>
#include "../util/simd.h"
#include "../stats/score_matrix.h"
#include "../util/simd/vector.h"

namespace DISPATCH_ARCH {

template<typename Score, int DELTA>
struct ScoreVector
{ };

template<typename Sv>
struct ScoreTraits
{
	typedef void Score;
	enum { CHANNELS = 0 };
};

template<>
struct ScoreTraits<int32_t>
{
	enum { CHANNELS = 1, BITS = 32 };
	typedef int32_t Score;
	struct TraceMask {
		static uint8_t make(int vmask, int hmask) {
			return vmask << 1 | hmask;
		}
		static uint8_t vmask(int channel) {
			return 2;
		}
		static uint8_t hmask(int channel) {
			return 1;
		}
		uint8_t gap;
		uint8_t open;
	};
	static int32_t zero()
	{
		return 0;
	}
	static int32_t zero_score()
	{
		return 0;
	}
	static int int_score(Score s)
	{
		return s;
	}
	template<typename T>
	static void saturate(T &x)
	{
		x = std::max((int32_t)x, 0);
	}
	static int max_score()
	{
		return INT_MAX;
	}
	static constexpr int max_int_score() {
		return INT_MAX;
	}
};

}

static inline void store_sv(int32_t sv, int32_t *dst)
{
	*dst = sv;
}

template<typename Sv>
static Sv load_sv(const typename DISPATCH_ARCH::ScoreTraits<Sv>::Score* ptr) {
	return Sv(ptr);
}

template<>
static int32_t load_sv<int32_t>(const int32_t* x) {
	return *x;
}

template<typename Sv>
static Sv blend_sv(const typename DISPATCH_ARCH::ScoreTraits<Sv>::Score a, const typename DISPATCH_ARCH::ScoreTraits<Sv>::Score b, const uint32_t mask) {
	const uint32_t CHANNELS = DISPATCH_ARCH::ScoreTraits<Sv>::CHANNELS;
	alignas(32) typename DISPATCH_ARCH::ScoreTraits<Sv>::Score s[CHANNELS];
	for (uint32_t i = 0; i < CHANNELS; ++i)
		if (mask & (1 << i))
			s[i] = b;
		else
			s[i] = a;
	return Sv(s);
}

template<>
static int32_t blend_sv<int32_t>(const int32_t a, const int32_t b, const uint32_t mask) {
	return mask ? b : a;
}

template<typename _t, typename _p, int DELTA>
static inline void store_sv(const DISPATCH_ARCH::ScoreVector<_t, DELTA> &sv, _p *dst)
{
#if ARCH_ID == 2
	_mm256_storeu_si256((__m256i*)dst, sv.data_);
#else
	_mm_storeu_si128((__m128i*)dst, sv.data_);
#endif
}

static inline int extract_channel(const int32_t v, const int i) {
	return v;
}

static inline int32_t set_channel(const int32_t v, const int i, const int32_t x) {
	return x;
}