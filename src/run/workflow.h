/****
DIAMOND protein aligner
Copyright (C) 2016-2020 Max Planck Society for the Advancement of Science e.V.
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
#include <list>
#include <memory>
#include "../util/data_structures/bit_vector.h"
#include "../util/scores/cutoff_table.h"

struct SequenceFile;
struct Consumer;
struct TextInputFile;
struct Block;
struct TaxonomyNodes;

namespace Search {

struct Config {

	Config(bool dealloc);
	void free();

	bool                      self;
	const bool                dealloc;
	SequenceFile*             db;
	Consumer*                 consumer;
	std::list<TextInputFile>* query_file;
	BitVector                 db_filter;
	TaxonomyNodes*            taxon_nodes;
	std::vector<std::string>* taxonomy_scientific_names;

	std::unique_ptr<Block> query, target;

	uint64_t db_seqs, db_letters, ref_blocks;
	Util::Scores::CutoffTable cutoff_gapped1, cutoff_gapped2;
	Util::Scores::CutoffTable2D cutoff_gapped1_new, cutoff_gapped2_new;

};

void run(Config &options);

}
