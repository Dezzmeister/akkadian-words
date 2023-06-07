#pragma once

#define UERR_FILE_NOT_FOUND	1
#define UERR_UNKNOWN		2

#include <string>

typedef enum {
	MissingWord,
	UnknownWordClass,
	UnknownGrammarKind,
	MissingRightParen,
	UnknownRelation,
	TooManyFields
} ParseErrorType;

typedef struct DictParseError {
	const int line;
	const ParseErrorType err_type;

	DictParseError(const int line, const ParseErrorType err_type);

	std::wstring message();

} DictParseError;
