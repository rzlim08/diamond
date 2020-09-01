/****
DIAMOND protein aligner
Copyright (C) 2013-2017 Benjamin Buchfink <buchfink@gmail.com>

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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include "tools.h"
#include "../basic/config.h"
#include "../data/sequence_set.h"
#include "../util/seq_file_format.h"
#include "../data/queries.h"
#include "../data/load_seqs.h"
#include "../data/reference.h"
#include "../basic/masking.h"
#include "../dp/dp.h"
#include "../basic/packed_transcript.h"

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

void get_seq()
{
	DatabaseFile db_file(config.database);
	db_file.get_seq();
}

void random_seqs()
{
	DatabaseFile db_file(config.database);
	db_file.load_seqs(nullptr, std::numeric_limits<size_t>::max(), &ref_seqs::data_, &ref_ids::data_);
	cout << "Sequences = " << ref_seqs::get().get_length() << endl;
	std::set<unsigned> n;
	const size_t count = atoi(config.seq_no[0].c_str());
	while (n.size() < count)
		n.insert((rand()*RAND_MAX + rand()) % ref_seqs::get().get_length());
	OutputFile out(config.output_file);
	unsigned j = 0;
	
	std::string s;
	for (std::set<unsigned>::const_iterator i = n.begin(); i != n.end(); ++i) {
		std::stringstream ss;
		ss << '>' << j++ << endl;
		if (config.reverse)
			; // ref_seqs::get()[*i].print(ss, value_traits, sequence::Reversed());
		else
			ss << ref_seqs::get()[*i];
		ss << endl;
		s = ss.str();
		out.write(s.data(), s.length());
	}
	out.close();
}

void sort_file()
{
	TextInputFile f(config.query_file);
	vector<Pair<unsigned, string> > data;
	while (f.getline(), !f.eof()) {
		unsigned query;
		sscanf(f.line.c_str(), "%u", &query);
		data.push_back(Pair<unsigned, string>(query, f.line));
	}
	std::stable_sort(data.begin(), data.end());
	for (vector<Pair<unsigned, string> >::const_iterator i = data.begin(); i != data.end(); ++i)
		cout << i->second << endl;
	f.close();
}

void db_stat()
{
	DatabaseFile db_file(config.database);
	db_file.load_seqs(nullptr, std::numeric_limits<size_t>::max(), &ref_seqs::data_, &ref_ids::data_);
	cout << "Sequences = " << ref_seqs::get().get_length() << endl;

	size_t letters = 0;
	vector<size_t> letter_freq(20);
	for (size_t i = 0; i < ref_seqs::get().get_length(); ++i) {
		const sequence seq = ref_seqs::get()[i];
		for (size_t j = 0; j < seq.length(); ++j) {
			if (seq[j] < 20) {
				++letters;
				++letter_freq[(int)seq[j]];
			}
		}
	}
	cout << "Frequencies = ";
	for (vector<size_t>::const_iterator i = letter_freq.begin(); i != letter_freq.end(); ++i)
		cout << (double)*i / letters << ',';
	cout << endl;

}

void run_masker()
{
	TextInputFile f(config.query_file);
	vector<Letter> seq, seq2;
	string id;
	const FASTA_format format;
	size_t letters = 0, seqs = 0, seqs_total = 0;

	high_resolution_clock::time_point t1 = high_resolution_clock::now();

	while (format.get_seq(id, seq, f, value_traits)) {
		cout << '>' << id << endl;
		seq2 = seq;
		Masking::get()(seq2.data(), seq2.size());
		/*for (size_t i = 0; i < seq.size(); ++i) {
			char c = value_traits.alphabet[(long)seq[i]];
			if (seq2[i] == value_traits.mask_char)
				c = tolower(c);
			cout << c;
		}
		cout << endl;*/
		cout << sequence(seq2.data(), seq2.size()) << endl;
		size_t n = 0;
		for (size_t i = 0; i < seq2.size(); ++i)
			if (seq2[i] == value_traits.mask_char) {
				++n;
			}
		letters += n;
		if (n > 0)
			++seqs;
		++seqs_total;
	}
	cerr << "#Sequences: " << seqs << "/" << seqs_total << ", #Letters: " << letters << ", t=" << (double)duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - t1).count()/1000.0 << endl;
}

void fastq2fasta()
{
	unique_ptr<TextInputFile> f(new TextInputFile(config.query_file));
	vector<Letter> seq;
	string id;
	const FASTQ_format format;
	input_value_traits = value_traits = nucleotide_traits;
	size_t n = 0, max = atoi(config.seq_no[0].c_str());
	while (n < max && format.get_seq(id, seq, *f, value_traits)) {
		cout << '>' << id << endl;
		cout << sequence(seq.data(), seq.size()) << endl;
		++n;
	}
}

void test_io()
{
	const size_t buf_size = 1;
	//Timer t;
	//t.start();
	/*InputFile f(config.query_file, InputFile::BUFFERED);
	char buf[buf_size];	
	size_t total = 0;
	while (f.read(buf, buf_size) == buf_size)
		total += buf_size;*/

	DatabaseFile db(config.database);
	db.load_seqs(nullptr, std::numeric_limits<size_t>::max(), &ref_seqs::data_, &ref_ids::data_);

	size_t total = ref_seqs::get().raw_len() + ref_ids::get().raw_len();
	//cout << "MBytes/sec = " << total / 1e6 / t.getElapsedTime() << endl;
	//cout << "Time = " << t.getElapsedTime() << "s" << endl;
}

void read_sim()
{
	const double ID = 0.35;
	srand((unsigned)time(0));
	TextInputFile in(config.query_file);
	OutputFile out(config.output_file);
	FASTA_format format;
	string id;
	vector<Letter> seq;
	input_value_traits = nucleotide_traits;
	TextBuffer buf;
	while (format.get_seq(id, seq, in, value_traits)) {
		buf << '>' << id << '\n';
		for (size_t i = 0; i < seq.size(); ++i) {
			if ((double)rand() / RAND_MAX <= ID)
				buf << nucleotide_traits.alphabet[(size_t)seq[i]];
			else
				buf << nucleotide_traits.alphabet[rand() % 4];
		}
		buf << '\n';
		out.write(buf.get_begin(), buf.size());
		buf.clear();
	}
	out.close();
}

void info()
{
	vector<string> arch_flags;
#ifdef __SSE2__
	arch_flags.push_back("sse2");
#endif
#ifdef __SSE3__
	arch_flags.push_back("sse3");
#endif
#ifdef __SSSE3__
	arch_flags.push_back("ssse3");
#endif
#ifdef __POPCNT__
	arch_flags.push_back("popcnt");
#endif

	cout << "Architecture flags: ";
	for (vector<string>::const_iterator i = arch_flags.begin(); i != arch_flags.end(); ++i)
		cout << *i << ' ';
	cout << endl;
}

void pairwise_worker(TextInputFile *in, std::mutex *input_lock, std::mutex *output_lock) {
	FASTA_format format;
	string id_r, id_q;
	vector<Letter> ref, query;

	while(true) {
		input_lock->lock();
		if (!format.get_seq(id_r, ref, *in, value_traits)) {
			input_lock->unlock();
			return;
		}
		if (!format.get_seq(id_q, query, *in, value_traits)) {
			input_lock->unlock();
			return;
		}
		input_lock->unlock();
		const string ir = blast_id(id_r), iq = blast_id(id_q);
		Hsp hsp;
		smith_waterman(sequence(query), sequence(ref), hsp);
		Hsp_context context(hsp, 0, TranslatedSequence(query), "", 0, 0, "", 0, 0, 0, sequence());
		Hsp_context::Iterator it = context.begin();
		std::stringstream ss;
		while (it.good()) {
			if (it.op() == op_substitution)
				ss << ir << '\t' << iq << '\t' << it.subject_pos << '\t' << it.query_pos.translated << '\t' << it.query_char() << endl;
			else if (it.op() == op_deletion)
				ss << ir << '\t' << iq << '\t' << it.subject_pos << '\t' << "-1" << '\t' << '-' << endl;
			++it;
		}
		output_lock->lock();
		cout << ss.str();
		output_lock->unlock();
	}
}

void pairwise()
{
	input_value_traits = nucleotide_traits;
	value_traits = nucleotide_traits;
	score_matrix = Score_matrix("DNA", 5, 2, 0, 1);

	TextInputFile in(config.query_file);
	std::mutex input_lock, output_lock;
	vector<thread> threads;
	for (unsigned i = 0; i < config.threads_; ++i)
		threads.emplace_back(pairwise_worker, &in, &input_lock, &output_lock);
	for (auto &t : threads)
		t.join();
}

void fasta_skip_to(string &id, vector<Letter> &seq, string &blast_id, TextInputFile &f)
{
	while (::blast_id(id) != blast_id) {
		if (!FASTA_format().get_seq(id, seq, f, value_traits))
			throw runtime_error("Sequence not found in FASTA file.");
	}
}

void translate() {
	input_value_traits = nucleotide_traits;
	TextInputFile in(config.query_file);
	string id;
	vector<Letter> seq;
	vector<Letter> proteins[6];
	while (FASTA_format().get_seq(id, seq, in, input_value_traits)) {
		Translator::translate(seq, proteins);
		cout << ">" << id << endl;
		cout << sequence(proteins[0]) << endl;
	}
	in.close();
}

void reverse() {
	input_value_traits = amino_acid_traits;
	TextInputFile in(config.query_file);
	string id;
	vector<Letter> seq;
	TextBuffer buf;
	while (FASTA_format().get_seq(id, seq, in, value_traits)) {
		buf << '>';
		buf << '\\';
		buf.write_raw(id.data(), id.size());
		buf << '\n';
		sequence(seq).print(buf, amino_acid_traits, sequence::Reversed());
		buf << '\n';
		buf << '\0';
		cout << buf.get_begin();
		buf.clear();
	}
	in.close();
}

void show_cbs() {
	score_matrix = Score_matrix("BLOSUM62", config.gap_open, config.gap_extend, config.frame_shift, 1);
	init_cbs();
	TextInputFile in(config.query_file);
	string id;
	vector<Letter> seq;
	while (FASTA_format().get_seq(id, seq, in, value_traits)) {
		Bias_correction bc{ sequence(seq) };
		for (size_t i = 0; i < seq.size(); ++i)
			cout << value_traits.alphabet[(long)seq[i]] << '\t' << bc[i] << endl;
	}
	in.close();
}