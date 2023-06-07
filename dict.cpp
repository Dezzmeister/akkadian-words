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

static std::pair<std::vector<WordClass>, std::vector<WordRelation>> parse_word_attrs(std::wstring& str, int line_num) {
	std::vector<std::wstring> tokens = split_str(str, ';');
	std::vector<WordClass> words;
	std::vector<WordRelation> rels;

	for (size_t i = 0; i < tokens.size(); i++) {
		std::wstring& token = tokens[i];
		size_t lpos = token.find('(');

		if (lpos == std::wstring::npos) {
			for (size_t j = 0; j < NUM_WORD_CLASSES; j++) {
				if (token == WORD_CLASSES[j]) {
					words.push_back((WordClass)j);
					goto cont;
				}
			}

			throw DictParseError(line_num, ParseErrorType::UnknownWordClass);
		} else {
			size_t rpos = token.find(')');

			if (rpos == std::wstring::npos) {
				throw DictParseError(line_num, ParseErrorType::MissingRightParen);
			}

			std::wstring rel_name = token.substr(0, lpos);
			WordRelationKind rel_kind;

			for (size_t j = 0; j < NUM_RELATIONS; j++) {
				if (rel_name == RELATIONS[j]) {
					rel_kind = (WordRelationKind)j;
					goto found_rel;
				}
			}

			throw DictParseError(line_num, ParseErrorType::UnknownRelation);
		found_rel:

			std::wstring word = token.substr(lpos + 1, rpos - lpos - 1);
			rels.push_back(WordRelation(rel_kind, word));
		}
	cont:;
	}

	return std::pair<std::vector<WordClass>, std::vector<WordRelation>>(words, rels);
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

static void resolve_relations(Dictionary& dict, std::wstring& word, std::vector<WordRelation>& rels) {
	for (size_t i = 0; i < rels.size(); i++) {
		WordRelation& rel = rels[i];

		if (rel.kind == WordRelationKind::PreteriteOf) {
			std::optional<DictEntry*> entry_opt = dict.get_akk(rel.word, GrammarKind::Verb, { WordClass::Infinitive });
			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown infinitive mapped by preterite: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::InfinitiveOf, word));
			}
		}
		else if (rel.kind == WordRelationKind::VerbalAdjOf) {
			std::optional<DictEntry*> entry_opt = dict.get_akk(rel.word, GrammarKind::Verb, { WordClass::Infinitive });
			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown infinitive mapped by verbal adj: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasVerbalAdj, word));
			}
		}
		else if (rel.kind == WordRelationKind::SubstOf) {
			std::optional<DictEntry*> entry_opt = dict.get_akk(rel.word, GrammarKind::Adjective, {});
			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown adjective mapped by substantivized noun: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasSubst, word));
			}
		}
	}
}

WordRelation::WordRelation(WordRelationKind kind, std::wstring word) : kind(kind), word(word) {}

bool WordRelation::operator<(const WordRelation& rhs) const {
	return kind < rhs.kind;
}

DictEntry::DictEntry(const std::vector<WordClass> word_types, const std::vector<std::wstring> defns, GrammarKind grammar_kind, std::vector<WordRelation> relations) : 
	word_types(word_types), defns(defns), grammar_kind(grammar_kind), relations(relations) {
	std::sort(this->word_types.begin(), this->word_types.end());
}

void DictEntry::add_relation(WordRelation rel) {
	for (size_t i = 0; i < relations.size(); i++) {
		WordRelation& r = relations[i];

		if (r.kind == rel.kind && r.word == rel.word) {
			return;
		}
	}

	relations.push_back(rel);
}

bool DictEntry::has_word_classes(std::vector<WordClass> classes) {
	return std::all_of(classes.begin(), classes.end(), [this](WordClass c) {
		return std::find(word_types.begin(), word_types.end(), c) != word_types.end();
	});
}

DictEntry DictEntry::merge(DictEntry& other) {
	std::vector<std::wstring> defns(this->defns);
	std::vector<WordRelation> relations(this->relations);

	defns.insert(defns.end(), other.defns.begin(), other.defns.end());
	relations.insert(relations.end(), other.relations.begin(), other.relations.end());

	return DictEntry(word_types, dedup(defns), grammar_kind, dedup(relations));
}

bool DictEntry::can_merge(DictEntry& other) {
	return grammar_kind == other.grammar_kind && word_types == other.word_types;
}

Dictionary::Dictionary() {
	std::random_device rd;
	this->rng = std::mt19937(rd());
}

std::optional<DictEntry*> Dictionary::get_akk(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes) {
	if (!akk_to_engl.count(word)) {
		return std::nullopt;
	}

	std::vector<DictEntry>& v = akk_to_engl[word];

	for (size_t i = 0; i < v.size(); i++) {
		DictEntry& d = v[i];

		if (d.grammar_kind == grammar_kind && d.has_word_classes(word_classes)) {
			return std::optional<DictEntry*>(&v[i]);
		}
	}
	
	return std::nullopt;
}

std::optional<DictEntry*> Dictionary::get_engl(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes) {
	if (!engl_to_akk.count(word)) {
		return std::nullopt;
	}

	std::vector<DictEntry>& v = engl_to_akk[word];

	for (size_t i = 0; i < v.size(); i++) {
		DictEntry& d = v[i];

		if (d.grammar_kind == grammar_kind && d.has_word_classes(word_classes)) {
			return std::optional<DictEntry*>(&v[i]);
		}
	}

	return std::nullopt;
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
	// Word relations are resolved after the entire dictionary has been read. This means that
	// a PreteriteOf relation can be defined before the corresponding infinitive, or a 
	// VerbalAdjOf before the infinitive, etc.
	std::vector<std::pair<std::wstring, std::vector<WordRelation>>> unresolved_rels;
	int line_num = 1;

	for (std::wstring line : lines) {
		std::vector<std::wstring> fields = split_str(line, ',');

		// Word class field is optional
		if (fields.size() < 3) {
			throw DictParseError(line_num, ParseErrorType::MissingWord);
		} else if (fields.size() > 4) {
			throw DictParseError(line_num, ParseErrorType::TooManyFields);
		}

		std::wstring akk_word = fields[0];
		std::vector<std::wstring> engl_words = split_str(fields[1], ';');
		GrammarKind grammar_kind = get_grammar_kind(fields[2], line_num);
		std::pair<std::vector<WordClass>, std::vector<WordRelation>> attrs;

		if (fields.size() > 3) {
			attrs = parse_word_attrs(fields[3], line_num);
			std::sort(attrs.first.begin(), attrs.first.end());
			unresolved_rels.push_back(std::pair<std::wstring, std::vector<WordRelation>>(akk_word, attrs.second));
		}

		DictEntry akk_entry(attrs.first, engl_words, grammar_kind, attrs.second);
		out.insert_akk(akk_word, akk_entry);

		for (std::wstring engl : engl_words) {
			DictEntry engl_entry(attrs.first, { akk_word }, grammar_kind, attrs.second);
			out.insert_engl(engl, engl_entry);
		}
	
		line_num++;
	}

	for (auto& rel_pair : unresolved_rels) {
		resolve_relations(out, rel_pair.first, rel_pair.second);
	}

	std::string debug_msg = "Read " + std::to_string(line_num - 1) + " lines\n";
	debug_msg += "Akk entries: " + std::to_string(out.akk_to_engl.size()) + "\n" +
		"English entries: " + std::to_string(out.engl_to_akk.size()) + "\n";

	OutputDebugStringA(debug_msg.c_str());

	return out;
}
