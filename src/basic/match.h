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
#include <limits>
#include <list>
#include "sequence.h"
#include "packed_loc.h"
#include "value.h"
#include "packed_transcript.h"
#include "translated_position.h"
#include "diagonal_segment.h"

inline interval normalized_range(unsigned pos, int len, Strand strand)
{
	return strand == FORWARD
			? interval (pos, pos + len)
			: interval (pos + 1 + len, pos + 1);
}

struct IntermediateRecord;

struct Hsp
{

	Hsp() :
		score(0),
		frame(0),
		length(0),
		identities(0),
		mismatches(0),
		positives(0),
		gap_openings(0),
		gaps(0),
		swipe_target(0),
		d_begin(0),
		d_end(0)
	{}

	Hsp(int score, int swipe_target = 0) :
		score(score),
		frame(0),
		length(0),
		identities(0),
		mismatches(0),
		positives(0),
		gap_openings(0),
		gaps(0),
		swipe_target(swipe_target),
		d_begin(0),
		d_end(0)
	{}

	Hsp(const IntermediateRecord &r, unsigned query_source_len);

	struct Iterator
	{
		Iterator(const Hsp &parent) :
			query_pos(parent.query_range.begin_, Frame(parent.frame)),
			subject_pos(parent.subject_range.begin_),
			ptr_(parent.transcript.ptr()),
			count_(ptr_->count())
		{ }
		bool good() const
		{
			return *ptr_ != Packed_operation::terminator();
		}
		Iterator& operator++()
		{
			switch (op()) {
			case op_deletion:
				++subject_pos;
				break;
			case op_insertion:
				++query_pos;
				break;
			case op_match:
			case op_substitution:
				++query_pos;
				++subject_pos;
				break;
			case op_frameshift_forward:
				query_pos.shift_forward();
				break;
			case op_frameshift_reverse:
				query_pos.shift_back();
				break;
			}
			--count_;
			if (count_ == 0) {
				++ptr_;
				count_ = ptr_->count();
			}
			return *this;
		}
		Edit_operation op() const
		{
			return ptr_->op();
		}
		bool to_next_match()
		{
			do {
				this->operator++();
			} while (good() && (op() == op_deletion || op() == op_insertion));
			return good();
		}
		TranslatedPosition query_pos;
		unsigned subject_pos;
	protected:
		const Packed_operation *ptr_;
		unsigned count_;
	};

	Iterator begin() const
	{
		return Iterator(*this);
	}

	interval oriented_range() const
	{
		if (frame < 3)
			return interval(query_source_range.begin_, query_source_range.end_ - 1);
		else
			return interval(query_source_range.end_ - 1, query_source_range.begin_);
	}

	void set_translated_query_begin(unsigned oriented_query_begin, unsigned dna_len)
	{
		int f = frame <= 2 ? frame + 1 : 2 - frame;
		if (f > 0)
			query_range.begin_ = (oriented_query_begin - (f - 1)) / 3;
		else
			query_range.begin_ = (dna_len + f - oriented_query_begin) / 3;
	}

	void set_translated_query_end(unsigned oriented_query_end, unsigned dna_len)
	{
		int f = frame <= 2 ? frame + 1 : 2 - frame;
		if (f > 0)
			query_range.end_ = (oriented_query_end - 2 - (f - 1)) / 3 + 1;
		else
			query_range.end_ = (dna_len + f - oriented_query_end) / 3 + 1;
	}

	int blast_query_frame() const
	{
		return align_mode.query_translated ? (frame <= 2 ? (int)frame + 1 : 2 - (int)frame) : 0;
	}

	bool operator<(const Hsp &rhs) const
	{
		return score > rhs.score || (score == rhs.score && query_source_range.begin_ < rhs.query_source_range.begin_);
	}

	double id_percent() const
	{
		return (double)identities * 100.0 / (double)length;
	}

	double query_cover_percent(unsigned query_source_len) const
	{
		return (double)query_source_range.length() * 100 / query_source_len;
	}

	double subject_cover_percent(unsigned subject_len) const
	{
		return (double)subject_range.length() * 100 / subject_len;
	}

	static bool cmp_query_pos(const Hsp &x, const Hsp &y)
	{
		return x.query_range.begin_ < y.query_range.begin_;
	}

	int partial_score(const Hsp &h) const
	{
		const double overlap = std::max(subject_range.overlap_factor(h.subject_range), query_source_range.overlap_factor(h.query_source_range));
		return int((1 - overlap)*score);
	}

	bool envelopes(const DiagonalSegment &d, int dna_len) const
	{
		return query_source_range.contains(d.query_absolute_range(dna_len)) || subject_range.contains(d.subject_range());
	}

	bool is_enveloped_by(const Hsp &hsp, double p) const;
	bool is_enveloped_by(std::list<Hsp>::const_iterator begin, std::list<Hsp>::const_iterator end, double p) const;
	bool is_weakly_enveloped_by(std::list<Hsp>::const_iterator begin, std::list<Hsp>::const_iterator end, int cutoff) const;
	void push_back(const DiagonalSegment &d, const TranslatedSequence &query, const Sequence& subject, bool reversed);
	void push_match(Letter q, Letter s, bool positive);
	void push_gap(Edit_operation op, int length, const Letter *subject);
	void splice(const DiagonalSegment &d0, const DiagonalSegment &d1, const TranslatedSequence &query, const Sequence& subject, bool reversed);
	void set_begin(const DiagonalSegment &d, int dna_len);
	void set_end(const DiagonalSegment &d, int dna_len);
	void set_begin(int i, int j, Frame frame, int dna_len);
	void set_end(int i, int j, Frame frame, int dna_len);
	void clear();

	bool is_weakly_enveloped(const Hsp &j) const;
	std::pair<int, int> diagonal_bounds() const;
	int score, frame, length, identities, mismatches, positives, gap_openings, gaps, swipe_target, d_begin, d_end;
	interval query_source_range, query_range, subject_range;
	double evalue, bit_score;
	Sequence target_seq;
	Packed_transcript transcript;
};

struct HspContext
{
	HspContext(
		Hsp& hsp,
		unsigned query_id,
		const TranslatedSequence &query,
		const char *query_title,
		unsigned subject_oid,
		unsigned subject_len,
		const char* subject_title,
		unsigned hit_num,
		unsigned hsp_num,
		const Sequence &subject_seq,
		int ungapped_score = 0
	) :
		query(query),
		query_title(query_title),
		target_title(subject_title),
		query_id(query_id),
		subject_oid(subject_oid),
		subject_len(subject_len),		
		hit_num(hit_num),
		hsp_num(hsp_num),
		ungapped_score(ungapped_score),
		subject_seq(subject_seq),
		hsp_(hsp)		
	{}
	struct Iterator : public Hsp::Iterator
	{
		Iterator(const HspContext &parent) :
			Hsp::Iterator(parent.hsp_),
			parent_(parent)
		{ }
		Letter query() const
		{
			return parent_.query[query_pos];
		}
		Letter subject() const
		{
			switch (op()) {
			case op_substitution:
			case op_deletion:
				return ptr_->letter();
			default:
				return query();
			}
		}
		char query_char() const
		{
			switch (op()) {
			case op_deletion:
				return '-';
			case op_frameshift_forward:
				return '\\';
			case op_frameshift_reverse:
				return '/';
			default:
				return value_traits.alphabet[(long)query()];
			}
		}
		char subject_char() const
		{
			switch (op()) {
			case op_insertion:
			case op_frameshift_forward:
			case op_frameshift_reverse:
				return '-';
			default:
				return value_traits.alphabet[(long)subject()];
			}
		}
		char midline_char(int score) const
		{
			switch (op()) {
			case op_match:
				return value_traits.alphabet[(long)query()];
			case op_substitution:
				return score > 0 ? '+' : ' ';
			default:
				return ' ';
			}
		}
	private:
		const HspContext &parent_;
	};
	Iterator begin() const
	{
		return Iterator(*this);
	}
	Packed_transcript::Const_iterator begin_old() const
	{
		return hsp_.transcript.begin();
	}
	unsigned score() const
	{ return hsp_.score; }
	double evalue() const
	{
		return hsp_.evalue;
	}
	double bit_score() const {
		return hsp_.bit_score;
	}
	unsigned frame() const
	{ return hsp_.frame; }
	unsigned length() const
	{ return hsp_.length; }
	unsigned identities() const
	{ return hsp_.identities; }
	unsigned mismatches() const
	{ return hsp_.mismatches; }
	unsigned positives() const
	{ return hsp_.positives; }
	unsigned gap_openings() const
	{ return hsp_.gap_openings; }
	unsigned gaps() const
	{ return hsp_.gaps; }
	const interval& query_source_range() const
	{ return hsp_.query_source_range; }
	const interval& query_range() const
	{ return hsp_.query_range; }
	const interval& subject_range() const
	{ return hsp_.subject_range; }
	interval oriented_query_range() const
	{ return hsp_.oriented_range(); }
	int blast_query_frame() const
	{ return hsp_.blast_query_frame(); }
	Packed_transcript transcript() const
	{ return hsp_.transcript; }
	HspContext& parse();

	const TranslatedSequence query;
	const char* query_title, *target_title;
	const unsigned query_id, subject_oid, subject_len, hit_num, hsp_num;
	int ungapped_score;
	const Sequence subject_seq;
private:	
	Hsp &hsp_;
};
