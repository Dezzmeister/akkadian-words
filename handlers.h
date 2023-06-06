#pragma once
#include <windows.h>

#include "dict.h"

typedef struct PracticeState {
	int correct;
	int total;
	std::wstring word;
	DictEntry entry;

	void reset();
	void new_word(Dictionary& dict, bool engl);
	bool accept_answer(std::wstring& answer);
	std::wstring get_summary(Dictionary& dict, bool engl, bool wasCorrect, std::wstring answer);
	std::wstring get_question();
} PracticeState;

INT_PTR CALLBACK PracticeEnglish(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK PracticeAkkadian(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
