/**
 * Akkadian/English translation practice features.
 * 
 * Author: Joe Desmond - desmonji@bc.edu
 */
#pragma once

#include "dict.h"

/**
 * State of a practice session for Akkadian or English words and short phrases. The user is given
 * a direct entry in the dictionary with appropriate word attributes, and the user has to give one of
 * the dictionary definitions. A point is given if the user's answer is one of the definitions.
 */
typedef struct WordPracticeState {
	int correct;
	int total;
	std::wstring word;
	DictEntry entry;

	// TODO: Artifact of static var in modal dialog box hander: remove in favor of centralized state
	// and modeless dialogs
	void reset();
	void new_word(Dictionary& dict, bool engl);
	bool accept_answer(std::wstring& answer);
	std::wstring get_summary(Dictionary& dict, bool engl, bool was_correct, std::wstring answer = L"");
	std::wstring get_question();
} PracticeState;

// TODO: Dative case and phrases with ana + anaphoric pronoun
typedef enum {
	Nom,
	Gen,
	Acc
} WordCase;

const int NUM_WORD_CASES = 3;

typedef struct PhraseAnswer {
	bool correct;
	std::wstring noun;
	std::wstring adj;

	PhraseAnswer(bool correct, std::wstring noun, std::wstring adj);
} PhraseAnswer;

/**
 * State of a practice session for Akkadian or English phrases. The phrases are made by choosing a random declension, then picking
 * a random noun and adjective (or rarely, the anaphoric pronoun) with that declension. The user is given the phrase and must provide
 * a correct translation for a point. 
 * 
 * There is a small chance that a noun in the dual case is chosen, which will cause a plural feminine adjective to be chosen. There is also a
 * small chance that the anaphoric pronoun is chosen instead of an adjective.
 * 
 * TODO: Prepositional phrases and bound form phrases
 */
typedef struct PhrasePracticeState {
	int correct;
	int total;
	WordCase word_case;
	std::wstring noun_word;
	std::wstring adj_word;
	DictEntry noun_entry;
	DictEntry adj_entry;

	void reset();
	// throws PracticeError
	void new_phrase(Dictionary& dict, bool engl);
	PhraseAnswer accept_answer(std::wstring& answer, bool engl);
	std::wstring get_summary(Dictionary& dict, bool engl, bool was_correct, std::wstring noun_answer = L"", std::wstring adj_answer = L"");
	std::wstring get_question(bool engl);
} PhrasePracticeState;
