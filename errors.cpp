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
	default: {
		return prefix + L"Unknown error";
	}
	}
}
