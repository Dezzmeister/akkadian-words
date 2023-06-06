#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <vector>
#include <windows.h>

#include "dict.h"
#include "errors.h"

Dictionary Akk::dict;

static bool file_exists(std::wstring& filename) {
	DWORD dwAttrib = GetFileAttributesW(filename.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static size_t file_size(std::wstring& filename) {
	struct _stat fileinfo;
	_wstat(filename.c_str(), &fileinfo);
	return fileinfo.st_size;
}

static std::vector<std::wstring> split_str(std::wstring& str, wchar_t delim) {
	std::vector<std::wstring> out;
	std::wstringstream stream(str);

	for (std::wstring token; std::getline(stream, token, delim);) {
		out.push_back(token);
	}

	return out;
}

static std::vector<WordClass> get_word_classes(std::wstring& str, int line_num) {
	std::vector<std::wstring> tokens = split_str(str, ';');
	std::vector<WordClass> out;

	std::transform(tokens.begin(), tokens.end(), std::back_inserter(out), [line_num](auto token) {
		for (size_t i = 0; i < NUM_WORD_CLASSES; i++) {
			if (token == WORD_CLASSES[i]) {
				return (WordClass)i;
			}
		}

		throw DictParseError(line_num, ParseErrorType::UnknownWordClass);
	});

	return out;
}

static GrammarKind get_grammar_kind(std::wstring& str, int line_num) {
	for (size_t i = 0; i < NUM_GRAMMAR_KINDS; i++) {
		if (str == GRAMMAR_KINDS[i]) {
			return (GrammarKind)i;
		}
	}

	throw DictParseError(line_num, ParseErrorType::UnknownGrammarKind);
}

template <typename T>
static std::vector<T> dedup(std::vector<T>& vec) {
	std::set<T> deduped(vec.begin(), vec.end());
	std::vector<T> out;
	out.assign(deduped.begin(), deduped.end());

	return out;
}

DictEntry::DictEntry(const std::vector<WordClass> word_types, const std::vector<std::wstring> defns, GrammarKind grammar_kind) : 
	word_types(word_types), defns(defns), grammar_kind(grammar_kind) {
	std::sort(this->word_types.begin(), this->word_types.end());
}

DictEntry DictEntry::merge(DictEntry& other) {
	std::vector<std::wstring> defns(this->defns);

	defns.insert(defns.end(), other.defns.begin(), other.defns.end());

	return DictEntry(word_types, dedup(defns), grammar_kind);
}

bool DictEntry::can_merge(DictEntry& other) {
	return grammar_kind == other.grammar_kind && word_types == other.word_types;
}

Dictionary::Dictionary() {
	std::random_device rd;
	this->rng = std::mt19937(rd());
}

void Dictionary::insert_engl(std::wstring engl, DictEntry entry) {
	if (!engl_to_akk.count(engl)) {
		engl_to_akk[engl] = { entry };
		engl_keys.push_back(engl);
		return;
	}

	std::vector<DictEntry>& existing_defns = engl_to_akk[engl];

	for (size_t i = 0; i < existing_defns.size(); i++) {
		if (entry.can_merge(existing_defns[i])) {
			DictEntry merged = existing_defns[i].merge(entry);
			existing_defns[i] = merged;
			return;
		}
	}

	existing_defns.push_back(entry);
}

void Dictionary::insert_akk(std::wstring akk, DictEntry entry) {
	if (!akk_to_engl.count(akk)) {
		akk_to_engl[akk] = { entry };
		akk_keys.push_back(akk);
		return;
	}

	std::vector<DictEntry>& existing_defns = akk_to_engl[akk];

	for (size_t i = 0; i < existing_defns.size(); i++) {
		if (entry.can_merge(existing_defns[i])) {
			DictEntry merged = existing_defns[i].merge(entry);
			existing_defns[i] = merged;
			return;
		}
	}

	existing_defns.push_back(entry);
}

std::pair<std::wstring, DictEntry> Dictionary::random_engl() {
	std::uniform_int_distribution<> keys_dist(0, engl_to_akk.size() - 1);
	int engl_index = keys_dist(rng);
	std::wstring engl = engl_keys[engl_index];
	std::vector<DictEntry>& entries = engl_to_akk[engl];
	std::uniform_int_distribution<> entries_dist(0, entries.size() - 1);
	int index = entries_dist(rng);
	DictEntry entry = entries[index];

	return std::make_pair(engl, entry);
}

std::pair<std::wstring, DictEntry> Dictionary::random_akk() {
	std::uniform_int_distribution<> keys_dist(0, akk_to_engl.size() - 1);
	int akk_index = keys_dist(rng);
	std::wstring akk = akk_keys[akk_index];
	std::vector<DictEntry>& entries = akk_to_engl[akk];
	std::uniform_int_distribution<> entries_dist(0, entries.size() - 1);
	int index = entries_dist(rng);
	DictEntry entry = entries[index];

	return std::make_pair(akk, entry);
}

Dictionary Akk::load_dict(std::wstring filename) {
	typedef std::codecvt_utf8<wchar_t> converter_type;

	if (!file_exists(filename)) {
		throw UERR_FILE_NOT_FOUND;
	}

	std::wstring buf;
	FILE* fp;
	errno_t code = _wfopen_s(&fp, filename.c_str(), L"rt, ccs=UTF-8");

	if (code) {
		throw UERR_UNKNOWN;
	}

	size_t size = file_size(filename);
	buf.resize(size);
	size_t chars_read = fread(&(buf.front()), sizeof(wchar_t), size, fp);
	buf.resize(chars_read);
	buf.shrink_to_fit();
	fclose(fp);
	std::vector<std::wstring> lines = split_str(buf, '\n');

	Dictionary out;
	int line_num = 1;

	for (std::wstring line : lines) {
		std::vector<std::wstring> fields = split_str(line, ',');

		// Word class field is optional
		if (fields.size() < 3) {
			throw DictParseError(line_num, ParseErrorType::MissingWord);
		}

		std::wstring akk_word = fields[0];
		std::vector<std::wstring> engl_words = split_str(fields[1], ';');
		GrammarKind grammar_kind = get_grammar_kind(fields[2], line_num);
		std::vector<WordClass> classes;

		if (fields.size() > 3) {
			classes = get_word_classes(fields[3], line_num);
		}

		DictEntry akk_entry(classes, engl_words, grammar_kind);
		out.insert_akk(akk_word, akk_entry);

		for (std::wstring engl : engl_words) {
			DictEntry engl_entry(classes, { akk_word }, grammar_kind);
			out.insert_engl(engl, engl_entry);
		}
	
		line_num++;
	}

	std::string debug_msg = "Read " + std::to_string(line_num - 1) + " lines\n";
	debug_msg += "Akk entries: " + std::to_string(out.akk_to_engl.size()) + "\n" +
		"English entries: " + std::to_string(out.engl_to_akk.size()) + "\n";

	OutputDebugStringA(debug_msg.c_str());

	return out;
}
