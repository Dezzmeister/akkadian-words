#pragma once

#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <map>
#include <optional>
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
	L"adv"
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
	L"G",
	L"id"
};

// These relations can be defined in the dictionary. The reverse relations will be set when parsing the dictionary.
// (For example, you can define a preterite of an infinitive, but not an infinitive of a preterite)
const std::wstring RELATIONS[] = {
	L"pret",
	L"va",
	L"subst"
};

const size_t NUM_GRAMMAR_KINDS = (sizeof GRAMMAR_KINDS) / (sizeof * GRAMMAR_KINDS);
const size_t NUM_WORD_CLASSES = (sizeof WORD_CLASSES) / (sizeof * WORD_CLASSES);
const size_t NUM_RELATIONS = (sizeof RELATIONS) / (sizeof * RELATIONS);

typedef enum {
	Noun,
	Pronoun,
	Adjective,
	Article,
	Conjunction,
	Preposition,
	Verb,
	Adverb
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
	GStem,
	Idiom
} WordClass;

typedef enum {
	PreteriteOf,
	VerbalAdjOf,
	// Substantivized adjective
	SubstOf,
	InfinitiveOf,
	HasSubst,
	HasVerbalAdj,
} WordRelationKind;

typedef struct WordRelation {
	WordRelationKind kind;
	std::wstring word;

	WordRelation(WordRelationKind kind, std::wstring word);

	bool operator<(const WordRelation& rhs) const;

} WordRelation;

typedef struct DictEntry {
	std::vector<WordClass> word_types;
	std::vector<std::wstring> defns;
	GrammarKind grammar_kind;
	std::vector<WordRelation> relations;

	DictEntry() = default;

	DictEntry(const std::vector<WordClass> word_types, const std::vector<std::wstring> defns, GrammarKind grammar_kind, std::vector<WordRelation> relations);

	void add_relation(WordRelation rel);

	bool has_word_classes(std::vector<WordClass> classes);

	DictEntry merge(DictEntry& other);
	bool can_merge(DictEntry& other);
} DictEntry;

typedef struct Dictionary {
	std::map<std::wstring, std::vector<DictEntry>> engl_to_akk;
	std::map<std::wstring, std::vector<DictEntry>> akk_to_engl;
	std::vector<std::wstring> engl_keys;
	std::vector<std::wstring> akk_keys;

	Dictionary();

	std::optional<DictEntry*> get_akk(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes);
	std::optional<DictEntry*> get_engl(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes);

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
