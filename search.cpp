/**
 * Search algorithms for the lookup feature. 
 *
 * When looking up an English word, every English entry is searched for the literal query. 
 * If a word contains the query, it is added to a list of candidate results. 
 * The candidates are sorted in ascending order by word length and the first N 
 * candidates are the results.
 * 
 * When looking up an Akkadian word, the length of the query is compared to a "cutoff." If
 * the query is shorter than the cutoff, then search method 1 is used, otherwise search method 2.
 * 
 * 1: Every Akkadian entry that is shorter than the query is discarded. The Akkadian entries
 * that start with the query string are the candidates. To determine if an entry starts with
 * the query string, diacritical marks are ignored. The candidates are sorted by string length
 * and the first N candidates are the results.
 * 
 * 2. The Levenshtein distance between every Akkadian entry and the query string is calculated.
 * In calculating this distance, the diacritical marks are significant. If the query string
 * and the entry are the same length, then the Hamming distance is calculated, ignoring
 * diacritical marks. The lower distance is compared to the cutoff. If the distance is
 * less than or equal to the cutoff, then the entry is a candidate. The candidates are sorted
 * by their distance (Levenshtein or Hamming), and the first N are the results.
 * 
 * ==========================================================================================
 * 
 * For the English search, you probably know how to spell the word/phrase you're looking for,
 * so an algorithm that looks for exact matches is fine. For the Akkadian search, you might
 * not know the exact diacritical marks, so the algorithm should be able to pick words that are
 * similar to your query word.
 * 
 * Author: Joe Desmond - dezzmeister16@gmail.com
 */
#include "common.h"
#include "dict.h"

/**
 * Returns true if the chars are equal when diacritical marks are removed.
 * This makes it possible to search for an 's' and get results with 'š'
 * and 'ṣ'.
 */
static bool cmp_chars(wchar_t a, wchar_t b) {
	switch (a) {
	case L'š':
	case L'ṣ':
	case L's':
		return b == L's' || b == L'š' || b == L'ṣ';
	case L't':
	case L'ṭ':
		return b == L't' || b == L'ṭ';
	case L'h':
	case L'ḫ':
		return b == L'h' || b == L'ḫ';
	case L'a':
	case L'ā':
	case L'â':
		return b == L'a' || b == L'ā' || b == L'â';
	case L'e':
	case L'ē':
	case L'ê':
		return b == L'e' || b == L'ē' || b == L'ê';
	case L'i':
	case L'ī':
	case L'î':
		return b == L'i' || b == L'ī' || b == L'î';
	case L'u':
	case L'ū':
	case L'û':
		return b == L'u' || b == L'ū' || b == L'û';
	}

	return a == b;
}

static bool akk_starts_with(const std::wstring& s, const std::wstring& sub) {
	if (s.size() < sub.size()) {
		return false;
	}

	for (size_t i = 0; i < sub.size(); i++) {
		if (!cmp_chars(sub[i], s[i])) {
			return false;
		}
	}

	return true;
}

/**
 * Levenshtein distance: a metric for comparing strings that does not require the strings
 * to have the same length. Algorithm adapted from 
 * https://www.codeproject.com/Articles/13525/Fast-memory-efficient-Levenshtein-algorithm-2.
 */
static int lev_dist(const std::wstring& s, const std::wstring& t) {
	int n = (int)s.size();
	int m = (int)t.size();
	int row_idx;
	int col_idx;
	wchar_t row_i;
	wchar_t col_j;
	int cost;

	if (n == 0) {
		return m;
	}

	if (m == 0) {
		return n;
	}

	std::vector<int> v0(n + 1);
	std::vector<int> v1(n + 1);
	std::vector<int> v_tmp;

	for (row_idx = 1; row_idx <= n; row_idx++) {
		v0[row_idx] = row_idx;
	}

	for (col_idx = 1; col_idx <= m; col_idx++) {
		v1[0] = col_idx;
		col_j = t[col_idx - 1];

		for (row_idx = 1; row_idx <= n; row_idx++) {
			row_i = s[row_idx - 1];

			if (cmp_chars(row_i, col_j)) {
				cost = 0;
			}
			else {
				cost = 1;
			}

			int m_min = v0[row_idx] + 1;
			int b = v1[row_idx - 1] + 1;
			int c = v0[row_idx - 1] + cost;

			if (b < m_min) {
				m_min = b;
			}

			if (c < m_min) {
				m_min = c;
			}

			v1[row_idx] = m_min;
		}

		v_tmp = v0;
		v0 = v1;
		v1 = v_tmp;
	}

	return v0[n];
}

static int hamming_dist(const std::wstring& s, const std::wstring& t) {
	int out = 0;

	for (size_t i = 0; i < s.size(); i++) {
		if (!cmp_chars(s[i], t[i])) {
			out += 1;
		}
	}

	return out;
}

std::vector<std::wstring> Dictionary::search(std::wstring& query, size_t limit, int cutoff, bool engl) const {
	if (engl) {
		return engl_search(query, limit);
	}

	if (query.size() <= cutoff) {
		return basic_search(query, limit);
	}

	return lev_search(query, limit, cutoff);
}

std::vector<std::wstring> Dictionary::engl_search(std::wstring& query, size_t limit) const {
	std::vector<std::wstring> out;
	
	for (const std::wstring& word : engl_keys) {
		if (word.find(query) != std::wstring::npos) {
			out.push_back(word);
		}
	}

	std::sort(out.begin(), out.end(), [](const std::wstring& lhs, const std::wstring& rhs) {
		return lhs.size() < rhs.size();
	});

	if (out.size() > limit) {
		out.resize(limit);
	}

	return out;
}

std::vector<std::wstring> Dictionary::lev_search(std::wstring& query, size_t limit, int cutoff) const {
	typedef std::pair<int, std::wstring> DistWord;

	std::vector<DistWord> results;

	for (const std::wstring& word : akk_keys) {
		int lev = lev_dist(query, word);

		// Prioritize substitutions at the start of the word
		if (query.size() <= word.size()) {
			int ham = hamming_dist(query, word);

			lev = min(lev, ham);
		}

		if (lev <= cutoff) {
			results.push_back(std::pair<int, std::wstring>(lev, word));
		}
	}

	std::sort(results.begin(), results.end(), [](const DistWord& lhs, const DistWord& rhs) {
		return lhs.first < rhs.first;
	});

	if (results.size() > limit) {
		results.resize(limit);
	}

	std::vector<std::wstring> out;
	std::transform(results.begin(), results.end(), std::back_inserter(out), [](const DistWord& item) {
		return item.second;
	});

	return out;
}

std::vector<std::wstring> Dictionary::basic_search(std::wstring& query, size_t limit) const {
	std::vector<std::wstring> out;

	for (const std::wstring& word : akk_keys) {
		if (akk_starts_with(word, query)) {
			out.push_back(word);
		}
	}

	std::sort(out.begin(), out.end(), [](const std::wstring& lhs, const std::wstring& rhs) {
		return lhs.size() < rhs.size();
	});

	if (out.size() > limit) {
		out.resize(limit);
	}

	return out;
}