/****
DIAMOND protein aligner
Copyright (C) 2016-2021 Max Planck Society for the Advancement of Science e.V.
                        Benjamin Buchfink
						
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
#include "score_vector.h"
#include "../util/simd.h"

namespace DISPATCH_ARCH {

#if ARCH_ID == 2

template<int DELTA>
struct ScoreVector<int16_t, DELTA>
{

	typedef __m256i Register;

	ScoreVector() :
		data_(::SIMD::_mm256_set1_epi16(DELTA))
	{}

	explicit ScoreVector(int x)
	{
		data_ = ::SIMD::_mm256_set1_epi16(x);
	}

	explicit ScoreVector(int16_t x)
	{
		data_ = ::SIMD::_mm256_set1_epi16(x);
	}

	explicit ScoreVector(__m256i data) :
		data_(data)
	{ }

	explicit ScoreVector(const int16_t* x) :
		data_(_mm256_loadu_si256((const __m256i*)x))
	{}

	explicit ScoreVector(const uint16_t* x) :
		data_(_mm256_loadu_si256((const __m256i*)x))
	{}

	ScoreVector(unsigned a, Register seq)
	{
		const __m256i* row_lo = reinterpret_cast<const __m256i*>(&score_matrix.matrix8u_low()[a << 5]);
		const __m256i* row_hi = reinterpret_cast<const __m256i*>(&score_matrix.matrix8u_high()[a << 5]);

		__m256i high_mask = _mm256_slli_epi16(_mm256_and_si256(seq, ::SIMD::_mm256_set1_epi8('\x10')), 3);
		__m256i seq_low = _mm256_or_si256(seq, high_mask);
		__m256i seq_high = _mm256_or_si256(seq, _mm256_xor_si256(high_mask, ::SIMD::_mm256_set1_epi8('\x80')));

		__m256i r1 = _mm256_load_si256(row_lo);
		__m256i r2 = _mm256_load_si256(row_hi);

		__m256i s1 = _mm256_shuffle_epi8(r1, seq_low);
		__m256i s2 = _mm256_shuffle_epi8(r2, seq_high);
		data_ = _mm256_and_si256(_mm256_or_si256(s1, s2), ::SIMD::_mm256_set1_epi16(255));
		data_ = _mm256_subs_epi16(data_, ::SIMD::_mm256_set1_epi16(score_matrix.bias()));
	}

	ScoreVector operator+(const ScoreVector& rhs) const
	{
		return ScoreVector(_mm256_adds_epi16(data_, rhs.data_));
	}

	ScoreVector operator-(const ScoreVector& rhs) const
	{
		return ScoreVector(_mm256_subs_epi16(data_, rhs.data_));
	}

	ScoreVector& operator+=(const ScoreVector& rhs) {
		data_ = _mm256_adds_epi16(data_, rhs.data_);
		return *this;
	}

	ScoreVector& operator-=(const ScoreVector& rhs)
	{
		data_ = _mm256_subs_epi16(data_, rhs.data_);
		return *this;
	}

	ScoreVector& operator &=(const ScoreVector& rhs) {
		data_ = _mm256_and_si256(data_, rhs.data_);
		return *this;
	}

	ScoreVector& operator++() {
		data_ = _mm256_adds_epi16(data_, ::SIMD::_mm256_set1_epi16(1));
		return *this;
	}

	ScoreVector& max(const ScoreVector& rhs)
	{
		data_ = _mm256_max_epi16(data_, rhs.data_);
		return *this;
	}

	friend ScoreVector blend(const ScoreVector&v, const ScoreVector&w, const ScoreVector&mask) {
		return ScoreVector(_mm256_blendv_epi8(v.data_, w.data_, mask.data_));
	}
	
	ScoreVector operator==(const ScoreVector&v) const {
		return ScoreVector(_mm256_cmpeq_epi16(data_, v.data_));
	}

	friend uint32_t cmp_mask(const ScoreVector&v, const ScoreVector&w) {
		return _mm256_movemask_epi8(_mm256_cmpeq_epi16(v.data_, w.data_));
	}

	friend ScoreVector max(const ScoreVector& lhs, const ScoreVector& rhs)
	{
		return ScoreVector(_mm256_max_epi16(lhs.data_, rhs.data_));
	}

	void store(int16_t* ptr) const
	{
		_mm256_storeu_si256((__m256i*)ptr, data_);
	}

	int16_t operator[](int i) const {
		int16_t d[16];
		store(d);
		return d[i];
	}

	ScoreVector& set(int i, int16_t x) {
		alignas(32) int16_t d[16];
		store(d);
		d[i] = x;
		data_ = _mm256_load_si256((__m256i*)d);
		return *this;
	}

	void expand_from_8bit() {
		__m128i in = _mm256_extractf128_si256(data_, 0);
		__m128i mask = _mm_set1_epi8((char)0x80);
		__m128i sign = _mm_cmpeq_epi8(_mm_and_si128(in, mask), mask);
		__m128i low = _mm_unpacklo_epi8(in, sign);
		__m128i hi = _mm_unpackhi_epi8(in, sign);
		data_ = _mm256_set_m128i(hi, low);
	}

	friend std::ostream& operator<<(std::ostream& s, ScoreVector v)
	{
		int16_t x[16];
		v.store(x);
		for (unsigned i = 0; i < 16; ++i)
			printf("%3i ", (int)x[i]);
		return s;
	}

	__m256i data_;

};

#elif defined(__SSE2__)

template<int DELTA>
struct ScoreVector<int16_t, DELTA>
{

	typedef __m128i Register;

	inline ScoreVector() :
		data_(::SIMD::_mm_set1_epi16(DELTA))
	{}

	explicit ScoreVector(int x)
	{
		data_ = ::SIMD::_mm_set1_epi16(x);
	}

	explicit ScoreVector(int16_t x)
	{
		data_ = ::SIMD::_mm_set1_epi16(x);
	}

	explicit ScoreVector(__m128i data) :
		data_(data)
	{ }
	
	explicit ScoreVector(const int16_t *x):
		data_(_mm_loadu_si128((const __m128i*)x))
	{}

	explicit ScoreVector(const uint16_t *x) :
		data_(_mm_loadu_si128((const __m128i*)x))
	{}

	ScoreVector(unsigned a, uint64_t seq)
	{
		const uint16_t* row((uint16_t*)&score_matrix.matrix16()[a << 5]);
		uint64_t b = uint64_t(row[seq & 0xff]);
		seq >>= 8;
		b |= uint64_t(row[seq & 0xff]) << 16;
		seq >>= 8;
		b |= uint64_t(row[seq & 0xff]) << 16 * 2;
		seq >>= 8;
		b |= uint64_t(row[seq & 0xff]) << 16 * 3;
		seq >>= 8;
		uint64_t c = uint64_t(row[seq & 0xff]);
		seq >>= 8;
		c |= uint64_t(row[seq & 0xff]) << 16;
		seq >>= 8;
		c |= uint64_t(row[seq & 0xff]) << 16 * 2;
		seq >>= 8;
		c |= uint64_t(row[seq & 0xff]) << 16 * 3;
		data_ = _mm_set_epi64x(c, b);
	}

	ScoreVector(unsigned a, Register seq)
	{
#ifdef __SSSE3__
		const __m128i *row = reinterpret_cast<const __m128i*>(&score_matrix.matrix8u()[a << 5]);

		__m128i high_mask = _mm_slli_epi16(_mm_and_si128(seq, ::SIMD::_mm_set1_epi8('\x10')), 3);
		__m128i seq_low = _mm_or_si128(seq, high_mask);
		__m128i seq_high = _mm_or_si128(seq, _mm_xor_si128(high_mask, ::SIMD::_mm_set1_epi8('\x80')));

		__m128i r1 = _mm_load_si128(row);
		__m128i r2 = _mm_load_si128(row + 1);
		__m128i s1 = _mm_shuffle_epi8(r1, seq_low);
		__m128i s2 = _mm_shuffle_epi8(r2, seq_high);
		data_ = _mm_and_si128(_mm_or_si128(s1, s2), ::SIMD::_mm_set1_epi16(255));
		data_ = _mm_subs_epi16(data_, ::SIMD::_mm_set1_epi16(score_matrix.bias()));
#endif
	}

	ScoreVector operator+(const ScoreVector&rhs) const
	{
		return ScoreVector(_mm_adds_epi16(data_, rhs.data_));
	}

	ScoreVector operator-(const ScoreVector&rhs) const
	{
		return ScoreVector(_mm_subs_epi16(data_, rhs.data_));
	}

	ScoreVector& operator+=(const ScoreVector& rhs) {
		data_ = _mm_adds_epi16(data_, rhs.data_);
		return *this;
	}

	ScoreVector& operator-=(const ScoreVector&rhs)
	{
		data_ = _mm_subs_epi16(data_, rhs.data_);
		return *this;
	}

	ScoreVector& operator &=(const ScoreVector& rhs) {
		data_ = _mm_and_si128(data_, rhs.data_);
		return *this;
	}

	ScoreVector& operator++() {
		data_ = _mm_adds_epi16(data_, ::SIMD::_mm_set1_epi16(1));
		return *this;
	}

	ScoreVector operator==(const ScoreVector&v) const {
		return ScoreVector(_mm_cmpeq_epi16(data_, v.data_));
	}

	friend uint32_t cmp_mask(const ScoreVector&v, const ScoreVector&w) {
		return (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi16(v.data_, w.data_));
	}

	ScoreVector& max(const ScoreVector&rhs)
	{
		data_ = _mm_max_epi16(data_, rhs.data_);
		return *this;
	}

	friend ScoreVector max(const ScoreVector& lhs, const ScoreVector&rhs)
	{
		return ScoreVector(_mm_max_epi16(lhs.data_, rhs.data_));
	}

	friend ScoreVector blend(const ScoreVector&v, const ScoreVector&w, const ScoreVector&mask) {
#ifdef __SSE4_1__
		return ScoreVector(_mm_blendv_epi8(v.data_, w.data_, mask.data_));
#else
		__m128i a = _mm_andnot_si128(mask.data_, v.data_);
		__m128i b = _mm_and_si128(mask.data_, w.data_);
		return ScoreVector(_mm_or_si128(a, b));
#endif
	}

	void store(int16_t *ptr) const
	{
		_mm_storeu_si128((__m128i*)ptr, data_);
	}

	int16_t operator[](int i) const {
		int16_t d[8];
		store(d);
		return d[i];
	}

	ScoreVector& set(int i, int16_t x) {
		int16_t d[8];
		store(d);
		d[i] = x;
		data_ = _mm_loadu_si128((__m128i*)d);
		return *this;
	}

	void expand_from_8bit() {
		__m128i mask = _mm_set1_epi8((char)0x80);
		__m128i sign = _mm_cmpeq_epi8(_mm_and_si128(data_, mask), mask);
		data_ = _mm_unpacklo_epi8(data_, sign);
	}

	friend std::ostream& operator<<(std::ostream& s, ScoreVector v)
	{
		int16_t x[8];
		v.store(x);
		for (unsigned i = 0; i < 8; ++i)
			printf("%3i ", (int)x[i]);
		return s;
	}

	__m128i data_;

};

#endif

#ifdef __SSE2__

template<int DELTA>
struct ScoreTraits<ScoreVector<int16_t, DELTA>>
{
	typedef ::DISPATCH_ARCH::SIMD::Vector<int16_t> Vector;
#if ARCH_ID == 2
	enum { CHANNELS = 16 };
	typedef uint16_t Mask;
	struct TraceMask {
		uint32_t gap;
		uint32_t open;
		static uint32_t make(uint32_t vmask, uint32_t hmask) {
			return (vmask & VMASK) | (hmask & HMASK);
		}
		static uint32_t vmask(int channel) {
			return 2 << (2 * channel);
		}
		static uint32_t hmask(int channel) {
			return 1 << (2 * channel);
		}
		static const uint32_t VMASK = 0xAAAAAAAAu, HMASK = 0x55555555u;
	};
#else
	enum { CHANNELS = 8 };
	typedef uint8_t Mask;
	struct TraceMask {
		static uint16_t make(uint16_t vmask, uint16_t hmask) {
			return (vmask & VMASK) | (hmask & HMASK);
		}
		static uint16_t vmask(int channel) {
			return 2 << (2 * channel);
		}
		static uint16_t hmask(int channel) {
			return 1 << (2 * channel);
		}
		uint16_t gap;
		uint16_t open;
		static const uint16_t VMASK = 0xAAAAu, HMASK = 0x5555u;
	};
#endif
	typedef int16_t Score;
	typedef uint16_t Unsigned;
	static ScoreVector<int16_t, DELTA> zero()
	{
		return ScoreVector<int16_t, DELTA>();
	}
	static void saturate(ScoreVector<int16_t, DELTA> &v)
	{
	}
	static constexpr int16_t zero_score()
	{
		return DELTA;
	}
	static int int_score(Score s)
	{
		return (int)s - DELTA;
	}
	static constexpr int16_t max_score()
	{
		return SHRT_MAX;
	}
	static constexpr int max_int_score() {
		return SHRT_MAX - DELTA;
	}
};

#endif

}

#ifdef __SSE2__

template<int DELTA>
static inline int16_t extract_channel(const DISPATCH_ARCH::ScoreVector<int16_t, DELTA>& v, int i) {
	return v[i];
}

template<int DELTA>
static inline DISPATCH_ARCH::ScoreVector<int16_t, DELTA> set_channel(const DISPATCH_ARCH::ScoreVector<int16_t, DELTA>& v, const int i, const int16_t x) {
	return DISPATCH_ARCH::ScoreVector<int16_t, DELTA>(v).set(i, x);
}

#endif