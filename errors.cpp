#include <cassert>

#include "errors.h"

DictParseError::DictParseError(const int line, const ParseErrorType err_type) : line(line), err_type(err_type) {}

std::wstring DictParseError::message() {
	std::wstring prefix = L"Line " + std::to_wstring(line) + L": ";

	switch (err_type) {
	case ParseErrorType::MissingWord: {
		return prefix + L"Missing one or more fields (Akkadian word, English definitions, and part of speech are required)";
	}
	case ParseErrorType::UnknownWordClass: {
		return prefix + L"Unknown word class";
	}
	case ParseErrorType::UnknownGrammarKind: {
		return prefix + L"Unknown part of speech";
	}
	case ParseErrorType::MissingRightParen: {
		return prefix + L"Missing closing right parenthesis";
	}
	case ParseErrorType::UnknownRelation: {
		return prefix + L"Unknown relation name";
	}
	case ParseErrorType::TooManyFields: {
		return prefix + L"Too many fields";
	}
	case ParseErrorType::FileNotFound: {
		return prefix + L"File not found";
	}
	case ParseErrorType::UnknownError: {
		return prefix + L"Unknown error";
	}
	default: {
		return prefix + L"Unknown error";
	}
	}
}

PracticeError::PracticeError(const PracticeErrorType err_type) : err_type(err_type) {}

std::wstring PracticeError::message() {
	switch (err_type) {
	case PracticeErrorType::NotEnoughCases: {
		return L"Dictionary needs to have nouns and adjectives in nom., gen., and acc., as well as masc., fem., sing., dual, and pl.";
	}
	default: {
		__assume(false);
	}
	}
}
