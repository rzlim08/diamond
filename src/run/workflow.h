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
#include <memory>
#include "../data/sequence_file.h"
#include "../util/io/text_input_file.h"
#include "../util/io/consumer.h"

namespace Search {

void run(const std::shared_ptr<SequenceFile>& db = nullptr, const std::shared_ptr<std::list<TextInputFile>>& query = nullptr, const std::shared_ptr<Consumer>& out = nullptr, const std::shared_ptr<BitVector>& db_filter = nullptr);

}
