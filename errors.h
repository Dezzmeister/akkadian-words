/**
 * Error types.
 * 
 * Author: Joe Desmond - desmonji@bc.edu
 */
#pragma once

#include <string>

typedef enum {
	MissingWord,
	UnknownWordClass,
	UnknownGrammarKind,
	MissingRightParen,
	UnknownRelation,
	TooManyFields,
	FileNotFound,
	UnknownError
} ParseErrorType;

typedef struct DictParseError {
	const int line;
	const ParseErrorType err_type;

	DictParseError(const int line, const ParseErrorType err_type);

	std::wstring message();

} DictParseError;
