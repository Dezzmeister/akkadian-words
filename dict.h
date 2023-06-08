/**
 * Types, constants, and declarations for the dictionary functionality. The Akkadian-English dictionary
 * is at the heart of this application.
 * 
 * Author: Joe Desmond - desmonji@bc.edu
 */
#pragma once

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
	// pret(nasāẖum) indicates that the current word is a preterite form of nasāẖum
	L"pret",
	// va(nasāẖum) indicates that the current word is a verbal adjective of nasāẖum
	L"va",
	// subst(nakrum) indicates that the current word is a substantivized noun form of nakrum
	L"subst"
};

const std::wstring RELATION_NAMES[] = {
	L"Preterite of",
	L"Verbal Adj. of",
	L"Substantivized N. of",
	L"Preterite",
	L"Substantivized",
	L"Verbal Adj."
};

const size_t NUM_GRAMMAR_KINDS = (sizeof GRAMMAR_KINDS) / (sizeof * GRAMMAR_KINDS);
const size_t NUM_WORD_CLASSES = (sizeof WORD_CLASSES) / (sizeof * WORD_CLASSES);
const size_t NUM_RELATIONS = (sizeof RELATIONS) / (sizeof * RELATIONS);
const size_t NUM_FULL_RELATIONS = (sizeof RELATION_NAMES) / (sizeof * RELATION_NAMES);

/**
 * Part of speech
 */
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
	HasPreterite,
	HasSubst,
	HasVerbalAdj,
} WordRelationKind;

typedef struct WordRelation {
	WordRelationKind kind;
	std::wstring word;

	WordRelation(WordRelationKind kind, std::wstring word);

	bool operator<(const WordRelation& rhs) const;

} WordRelation;

/**
 * An single entry for a word. An entry can have multiple WordClasses and multiple definitions.
 * A single dictionary entry has one part of speech, and can be related to other words in various
 * ways.
 */
typedef struct DictEntry {
	std::vector<WordClass> word_types;
	std::vector<std::wstring> defns;
	GrammarKind grammar_kind;
	std::vector<WordRelation> relations;

	DictEntry() = default;

	DictEntry(const std::vector<WordClass> word_types, const std::vector<std::wstring> defns, GrammarKind grammar_kind, std::vector<WordRelation> relations);

	void add_relation(WordRelation rel);

	bool has_word_classes(std::vector<WordClass> classes) const;

	/**
	 * Generate a summary of the dict entry for displaying as a search result. Uses the \r\n line separator
	 * because plain \n doesn't work with edit controls.
	 */
	std::wstring akk_summary(std::wstring& word) const;

	/**
	 * Generates a summary of the dict entry. The summary is shorter for an English word. Uses the \r\n line 
	 * separator because plain \n doesn't work with edit controls.
	 */
	std::wstring engl_summary(std::wstring& word) const;

	DictEntry merge(DictEntry& other) const;
	bool can_merge(DictEntry& other) const;
} DictEntry;

/**
 * The core data structure of the application. A Dictionary is really two dictionaries, one from Akkadian
 * to English and the other from English to Akkadian. The dictionary is constructed from a file that maps 
 * Akkadian words to English definitions. Constructing the Akk->Engl dictionary from this is straightforward.
 * The Engl->Akk dictionary is constructed by mapping each definition given for a word back to the word.
 * Word classes/attributes and part of speech are preserved. If two Akkadian words have the same part of
 * speech and share an English definition, their corresponding Engl->Akk dictionary will be merged,
 * so that the English definition maps to both Akkadian words.
 * 
 * Each key word in a dictionary may have several entries. For example, 'nakrum' is both an adjective and a
 * noun (substantivized). These should be separate definitions, so the key 'nakrum' in the Akk->Engl dictionary
 * will have two DictEntries. The fact that one is a substantivization of the other is represented with a
 * bidirectional relation (see WordRelation).
 * 
 * The keys are kept separately in vectors to allow efficient random selection of keys. This is used for
 * the practice functionality. Note that the key word chosen follows a uniform distribution, but the dict
 * entry chosen does not, because words can map to more than one dict entry.
 */
typedef struct Dictionary {
	Dictionary() = default;

	/**
	 * Loads the dictionary from a file following a CSV format. Each line of the file should have 3 or 4
	 * comma-separated fields. The fields are as follows:
	 * 
	 *		1. Akkadian word
	 *		2. English definitions (semicolon-separated)
	 *		3. Part of speech (any of the strings in GRAMMAR_KINDS)
	 *		4. Word classes and relations (semicolon-separated, optional field)
	 * 
	 * A valid word class is any one of the strings in WORD_CLASSES. A valid relation is any one of the strings
	 * in RELATIONS, followed by a left paren, a word, and a right paren. Relations are set in one direction in the
	 * file, and the corresponding reverse relations are determined when the file is loaded. See RELATIONS for more info.
	 * 
	 * Whitespace is significant, even at the beginning and end of a line.
	 */
	Dictionary(std::wstring filename);

	std::optional<const std::vector<DictEntry>*> get_akk(std::wstring& akk) const;
	std::optional<const std::vector<DictEntry>*> get_engl(std::wstring& engl) const;

	/**
	 * Searches all English entries and returns results in ascending order of Levenshtein
	 * distance. The number of items returned is at most 'limit', and the returned vector
	 * will contain no words that have a Levenshtein distance from the query word that is greater
	 * than 'cutoff'. See search.cpp for an explanation of the search algorithm.
	 */
	std::vector<std::wstring> search(std::wstring& query, size_t limit, int cutoff, bool engl) const;

	std::pair<std::wstring, DictEntry> random_engl();
	std::pair<std::wstring, DictEntry> random_akk();

	std::wstring akk_summary(std::wstring& akk) const;
	std::wstring engl_summary(std::wstring& engl) const;

private:
	std::map<std::wstring, std::vector<DictEntry>> engl_to_akk;
	std::map<std::wstring, std::vector<DictEntry>> akk_to_engl;
	std::vector<std::wstring> engl_keys;
	std::vector<std::wstring> akk_keys;
	std::mt19937 rng;

	std::vector<std::wstring> lev_search(std::wstring& query, size_t limit, int cutoff) const;
	std::vector<std::wstring> basic_search(std::wstring& query, size_t limit, int cutoff) const;
	std::vector<std::wstring> engl_search(std::wstring& query, size_t limit) const;

	void resolve_relations(std::wstring& word, std::vector<WordRelation>& rels);

	void insert_engl(std::wstring engl, DictEntry entry);
	void insert_akk(std::wstring akk, DictEntry entry);

	std::optional<DictEntry*> get_akk_filters(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes);
	std::optional<DictEntry*> get_engl_filters(std::wstring& word, GrammarKind grammar_kind, std::vector<WordClass> word_classes);
} Dictionary;

namespace Akk {
	extern Dictionary dict;
	Dictionary load_dict(std::wstring filename);
}
