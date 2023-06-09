#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <tuple>
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

WordRelation::WordRelation(WordRelationKind kind, std::wstring word) : kind(kind), word(word) {}

bool WordRelation::operator<(const WordRelation& rhs) const {
	return kind < rhs.kind;
}

DictEntry::DictEntry(const std::vector<WordClass> word_types, const std::vector<std::wstring> defns, GrammarKind grammar_kind, std::vector<WordRelation> relations) : 
	word_types(word_types), defns(defns), grammar_kind(grammar_kind), relations(relations) {
	std::sort(this->word_types.begin(), this->word_types.end());
	std::sort(this->relations.begin(), this->relations.end());
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

bool DictEntry::has_word_classes(std::vector<WordClass> classes) const {
	return std::all_of(classes.begin(), classes.end(), [this](WordClass c) {
		return std::find(word_types.begin(), word_types.end(), c) != word_types.end();
	});
}

std::wstring DictEntry::akk_summary(std::wstring& word) const {
	std::wstring out = word + L" (" + GRAMMAR_KINDS[grammar_kind];

	if (word_types.size() > 0) {
		out += L"; ";

		for (size_t i = 0; i < word_types.size() - 1; i++) {
			out += WORD_CLASSES[word_types[i]] + L", ";
		}

		out += WORD_CLASSES[word_types[word_types.size() - 1]];
	}

	out += L"):\r\n";

	for (size_t i = 0; i < defns.size() - 1; i++) {
		out += defns[i] + L", ";
	}

	out += defns[defns.size() - 1] + L"\r\n";

	std::vector<std::wstring> buckets[NUM_FULL_RELATIONS];

	for (const WordRelation& rel : relations) {
		buckets[rel.kind].push_back(rel.word);
	}

	for (size_t i = 0; i < NUM_FULL_RELATIONS; i++) {
		if (!buckets[i].size()) {
			continue;
		}

		out += RELATION_NAMES[i] + L": ";

		for (size_t j = 0; j < buckets[i].size() - 1; j++) {
			out += buckets[i][j] + L", ";
		}

		out += buckets[i][buckets[i].size() - 1] + L"\r\n";
	}

	return out;
}

std::wstring DictEntry::engl_summary(std::wstring& word) const {
	std::wstring out = word + L" (" + GRAMMAR_KINDS[grammar_kind];

	if (word_types.size() > 0) {
		out += L"; ";

		for (size_t i = 0; i < word_types.size() - 1; i++) {
			out += WORD_CLASSES[word_types[i]] + L", ";
		}

		out += WORD_CLASSES[word_types[word_types.size() - 1]];
	}

	out += L"):\r\n";

	for (size_t i = 0; i < defns.size() - 1; i++) {
		out += defns[i] + L", ";
	}

	out += defns[defns.size() - 1] + L"\r\n";

	return out;
}

DictEntry DictEntry::merge(DictEntry& other) const {
	std::vector<std::wstring> defns(this->defns);
	std::vector<WordRelation> relations(this->relations);

	defns.insert(defns.end(), other.defns.begin(), other.defns.end());
	relations.insert(relations.end(), other.relations.begin(), other.relations.end());

	return DictEntry(word_types, dedup(defns), grammar_kind, dedup(relations));
}

bool DictEntry::can_merge(DictEntry& other) const {
	return grammar_kind == other.grammar_kind && word_types == other.word_types;
}

Dictionary::Dictionary(std::wstring filename) {
	if (!file_exists(filename)) {
		throw DictParseError(0, ParseErrorType::FileNotFound);
	}

	std::wstring buf;
	FILE* fp;
	errno_t code = _wfopen_s(&fp, filename.c_str(), L"rt, ccs=UTF-8");

	if (code) {
		throw DictParseError(0, ParseErrorType::UnknownError);
	}

	size_t size = file_size(filename);
	buf.resize(size);
	size_t chars_read = fread(&(buf.front()), sizeof(wchar_t), size, fp);
	buf.resize(chars_read);
	buf.shrink_to_fit();
	fclose(fp);
	std::vector<std::wstring> lines = split_str(buf, '\n');

	std::random_device rd;
	this->rng = std::mt19937(rd());

	// Word relations are resolved after the entire dictionary has been read. This means that
	// a PreteriteOf relation can be defined before the corresponding infinitive, or a 
	// VerbalAdjOf before the infinitive, etc.
	std::vector<std::tuple<std::wstring, GrammarKind, std::vector<WordRelation>>> unresolved_rels;
	int line_num = 1;

	for (std::wstring line : lines) {
		std::vector<std::wstring> fields = split_str(line, ',');

		// Word class field is optional
		if (fields.size() < 3) {
			throw DictParseError(line_num, ParseErrorType::MissingWord);
		}
		else if (fields.size() > 4) {
			throw DictParseError(line_num, ParseErrorType::TooManyFields);
		}

		std::wstring akk_word = fields[0];
		std::vector<std::wstring> engl_words = split_str(fields[1], ';');
		GrammarKind grammar_kind = get_grammar_kind(fields[2], line_num);
		std::pair<std::vector<WordClass>, std::vector<WordRelation>> attrs;

		if (fields.size() > 3) {
			attrs = parse_word_attrs(fields[3], line_num);
			std::sort(attrs.first.begin(), attrs.first.end());

			unresolved_rels.push_back(std::tuple<std::wstring, GrammarKind, std::vector<WordRelation>>(akk_word, grammar_kind, attrs.second));
		}

		DictEntry akk_entry(attrs.first, engl_words, grammar_kind, attrs.second);
		insert_akk(akk_word, akk_entry);

		for (std::wstring engl : engl_words) {
			DictEntry engl_entry(attrs.first, { akk_word }, grammar_kind, attrs.second);
			insert_engl(engl, engl_entry);
		}

		line_num++;
	}

	for (auto& [word, grammar_kind, rels] : unresolved_rels) {
		resolve_relations(word, grammar_kind, rels);
	}

	std::sort(akk_keys.begin(), akk_keys.end());
	std::sort(engl_keys.begin(), engl_keys.end());

	std::string debug_msg = "Read " + std::to_string(line_num - 1) + " lines\n";
	debug_msg += "Akk entries: " + std::to_string(akk_to_engl.size()) + "\n" +
		"English entries: " + std::to_string(engl_to_akk.size()) + "\n";

	OutputDebugStringA(debug_msg.c_str());
}

std::optional<const std::vector<DictEntry>*> Dictionary::get_akk(std::wstring& akk) const {
	if (!akk_to_engl.count(akk)) {
		return std::nullopt;
	}

	const std::vector<DictEntry>* entry = &akk_to_engl.at(akk);

	return std::optional<const std::vector<DictEntry>*>(entry);
}

std::optional<const std::vector<DictEntry>*> Dictionary::get_engl(std::wstring& engl) const {
	if (!engl_to_akk.count(engl)) {
		return std::nullopt;
	}

	const std::vector<DictEntry>* entry = &engl_to_akk.at(engl);

	return std::optional<const std::vector<DictEntry>*>(entry);
}

std::optional<DictEntry*> Dictionary::get_akk_filters(std::wstring& word, std::vector<GrammarKind> kinds, std::vector<WordClass> word_classes) {
	if (!akk_to_engl.count(word)) {
		return std::nullopt;
	}

	std::vector<DictEntry>& v = akk_to_engl[word];

	for (size_t i = 0; i < v.size(); i++) {
		DictEntry& d = v[i];

		const bool has_kind = std::find(kinds.begin(), kinds.end(), d.grammar_kind) != kinds.end();

		if (has_kind && d.has_word_classes(word_classes)) {
			return std::optional<DictEntry*>(&v[i]);
		}
	}
	
	return std::nullopt;
}

std::optional<DictEntry*> Dictionary::get_engl_filters(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes) {
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

std::wstring Dictionary::akk_summary(std::wstring& akk) const {
	if (!akk_to_engl.count(akk)) {
		return L"Unknown word";
	}

	const std::vector<DictEntry>& entries = akk_to_engl.at(akk);

	std::wstring out;

	for (const DictEntry& entry : entries) {
		out += entry.akk_summary(akk) + L"\r\n";
	}

	return out;
}

std::wstring Dictionary::engl_summary(std::wstring& engl) const {
	if (!engl_to_akk.count(engl)) {
		return L"Unknown word";
	}

	const std::vector<DictEntry>& entries = engl_to_akk.at(engl);

	std::wstring out;

	for (const DictEntry& entry : entries) {
		out += entry.engl_summary(engl) + L"\r\n";
	}

	return out;

}

std::pair<std::wstring, DictEntry> Dictionary::random_engl() {
	std::uniform_int_distribution<> keys_dist(0, engl_to_akk.size() - 1);
	int engl_index = keys_dist(rng);
	std::wstring engl = engl_keys[engl_index];
	const std::vector<DictEntry>& entries = engl_to_akk.at(engl);
	std::uniform_int_distribution<> entries_dist(0, entries.size() - 1);
	int index = entries_dist(rng);
	DictEntry entry = entries[index];

	return std::make_pair(engl, entry);
}

std::pair<std::wstring, DictEntry> Dictionary::random_akk() {
	std::uniform_int_distribution<> keys_dist(0, akk_to_engl.size() - 1);
	int akk_index = keys_dist(rng);
	std::wstring akk = akk_keys[akk_index];
	const std::vector<DictEntry>& entries = akk_to_engl.at(akk);
	std::uniform_int_distribution<> entries_dist(0, entries.size() - 1);
	int index = entries_dist(rng);
	DictEntry entry = entries[index];

	return std::make_pair(akk, entry);
}

void Dictionary::resolve_relations(std::wstring& word, GrammarKind grammar_kind, std::vector<WordRelation>& rels) {
	for (size_t i = 0; i < rels.size(); i++) {
		WordRelation& rel = rels[i];

		if (rel.kind == WordRelationKind::PreteriteOf) {
			std::optional<DictEntry*> entry_opt = get_akk_filters(rel.word, { GrammarKind::Verb }, { WordClass::Infinitive });
			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown infinitive mapped by preterite: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasPreterite, word));
			}
		}
		else if (rel.kind == WordRelationKind::VerbalAdjOf) {
			std::optional<DictEntry*> entry_opt = get_akk_filters(rel.word, { GrammarKind::Verb }, { WordClass::Infinitive });
			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown infinitive mapped by verbal adj: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasVerbalAdj, word));
			}
		}
		else if (rel.kind == WordRelationKind::SubstOf) {
			std::optional<DictEntry*> entry_opt = get_akk_filters(rel.word, { GrammarKind::Adjective }, {});
			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown adjective mapped by substantivized noun: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasSubst, word));
			}
		}
		else if (rel.kind == WordRelationKind::BoundFormOf) {
			std::optional<DictEntry*> entry_opt_n = get_akk_filters(rel.word, { grammar_kind }, {});
			std::optional<DictEntry*> entry_opt_v = get_akk_filters(rel.word, { GrammarKind::Verb }, { WordClass::Infinitive });

			if (!entry_opt_n.has_value() && !entry_opt_v.has_value()) {
				OutputDebugStringW((L"Unknown n/adj/v mapped by bound form: " + rel.word + L"\n").c_str());
			}
			else if (entry_opt_n.has_value()) {
				DictEntry* entry = *entry_opt_n;

				entry->add_relation(WordRelation(WordRelationKind::HasBoundForm, word));
			}
			else {
				DictEntry* entry = *entry_opt_v;

				entry->add_relation(WordRelation(WordRelationKind::HasBoundForm, word));
			}
		}
		else if (rel.kind == WordRelationKind::GenitiveOf) {
			std::optional<DictEntry*> entry_opt = get_akk_filters(rel.word, { grammar_kind }, { WordClass::Nominative });

			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown n/adj mapped by genitive case: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasGenitive, word));
			}
		}
		else if (rel.kind == WordRelationKind::AccusativeOf) {
			std::optional<DictEntry*> entry_opt = get_akk_filters(rel.word, { grammar_kind }, { WordClass::Nominative });

			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown n/adj mapped by accusative case: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasAccusative, word));
			}
		}
		else if (rel.kind == WordRelationKind::DativeOf) {
			std::optional<DictEntry*> entry_opt = get_akk_filters(rel.word, { grammar_kind }, { WordClass::Nominative });

			if (!entry_opt.has_value()) {
				OutputDebugStringW((L"Unknown n/adj/pr mapped by dative case: " + rel.word + L"\n").c_str());
			}
			else {
				DictEntry* entry = *entry_opt;

				entry->add_relation(WordRelation(WordRelationKind::HasDative, word));
			}
		}
	}
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
