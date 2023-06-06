#pragma once

#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <map>
#include <random>
#include <string>
#include <vector>

const std::wstring GRAMMAR_KINDS[] = {
	L"n",
	L"pr",
	L"adj",
	L"art",
	L"conj",
	L"prep",
	L"v",
	L"id"
};

const std::wstring WORD_CLASSES[] = {
	L"m",
	L"f",
	L"s",
	L"dual",
	L"pl",
	L"nom",
	L"gen",
	L"acc",
	L"inf",
	L"pret",
	L"G"
};

const size_t NUM_GRAMMAR_KINDS = (sizeof GRAMMAR_KINDS) / (sizeof * GRAMMAR_KINDS);
const size_t NUM_WORD_CLASSES = (sizeof WORD_CLASSES) / (sizeof * WORD_CLASSES);

typedef enum {
	Noun,
	Pronoun,
	Adjective,
	Article,
	Preposition,
	Verb,
	Idiom
} GrammarKind;

typedef enum {
	Masculine,
	Feminine,
	Singular,
	Dual,
	Plural,
	Nominative,
	Genitive,
	Accusative,
	Infinitive,
	Preterite,
	GStem
} WordClass;

typedef struct DictEntry {
	std::vector<WordClass> word_types;
	std::vector<std::wstring> defns;
	GrammarKind grammar_kind;

	DictEntry() = default;

	DictEntry(const std::vector<WordClass> word_types, const std::vector<std::wstring> defns, GrammarKind grammar_kind);

	DictEntry merge(DictEntry& other);
	bool can_merge(DictEntry& other);
} DictEntry;

typedef struct Dictionary {
	std::map<std::wstring, std::vector<DictEntry>> engl_to_akk;
	std::map<std::wstring, std::vector<DictEntry>> akk_to_engl;
	std::vector<std::wstring> engl_keys;
	std::vector<std::wstring> akk_keys;

	Dictionary();

	void insert_engl(std::wstring engl, DictEntry entry);
	void insert_akk(std::wstring akk, DictEntry entry);
	std::pair<std::wstring, DictEntry> random_engl();
	std::pair<std::wstring, DictEntry> random_akk();

private:
	std::mt19937 rng;
} Dictionary;

namespace Akk {
	extern Dictionary dict;
	Dictionary load_dict(std::wstring filename);
}
