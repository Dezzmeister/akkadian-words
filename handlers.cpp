#include <algorithm>
#include <assert.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <windowsx.h>

#include "common.h"
#include "components.h"
#include "errors.h"
#include "handlers.h"
#include "resource.h"
#include "practice.h"

static std::wstring get_input_txt(HWND hdlg, int res_id) {
    wchar_t buf[MAX_ANSWER_CHARS + 1];
    int chars_read = GetDlgItemTextW(hdlg, res_id, buf, MAX_ANSWER_CHARS);
    buf[chars_read] = '\0';

    return std::wstring(buf);
}

/**
 * This definitely doesn't cover every whitespace character but it's good enough for our purposes
 */
static bool is_whitespace(wchar_t c) {
    return c == L' ' || c == L'\r' || c == L'\n' || c == L'\t';
}

static std::wstring trim(std::wstring& str) {
    int start = 0;
    int end = str.size();

    while (is_whitespace(str[start])) start++;
    while (is_whitespace(str[end - 1])) end--;

    if (end < start) {
        return L"";
    }

    return str.substr(start, end - start);
}

static INT_PTR CALLBACK PracticeWordsDialog(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param, bool engl) {
    UNREFERENCED_PARAMETER(l_param);

    static WordPracticeState state;

    HWND word_hwnd = GetDlgItem(hdlg, IDC_WORD);
    HWND summary_hwnd = GetDlgItem(hdlg, IDC_SUMMARY);
    HWND answer_hwnd = GetDlgItem(hdlg, IDC_ANSWER);
    HWND your_answer_hwnd = GetDlgItem(hdlg, IDC_YOUR_ANSWER);

    switch (message) {
    case WM_INITDIALOG: {
        Edit_LimitText(answer_hwnd, MAX_ANSWER_CHARS);
        SetWindowSubclass(answer_hwnd, AkkadianEditControl, 0, NULL);
        state.reset();
        state.new_word(Akk::dict, engl);
        SetWindowTextW(word_hwnd, state.get_question().c_str());
        SetWindowTextW(summary_hwnd, state.get_summary(Akk::dict, engl, false).c_str());
        SetWindowTextW(answer_hwnd, L"");
        SetWindowTextW(your_answer_hwnd, L"");
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(w_param) == IDCANCEL) {
            EndDialog(hdlg, LOWORD(w_param));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(w_param) == IDOK) {
            std::wstring answer = get_input_txt(hdlg, IDC_ANSWER);
            bool correct = state.accept_answer(answer);
            SetWindowTextW(summary_hwnd, state.get_summary(Akk::dict, engl, correct, answer).c_str());
            state.new_word(Akk::dict, engl);
            SetWindowTextW(word_hwnd, state.get_question().c_str());
            SetWindowTextW(answer_hwnd, L"");
            SetWindowTextW(your_answer_hwnd, (L"Your answer: " + answer).c_str());
            return (INT_PTR)TRUE;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}

static INT_PTR CALLBACK PracticePhrasesDialog(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param, bool engl) {
    UNREFERENCED_PARAMETER(l_param);

    static PhrasePracticeState state;

    HWND word_hwnd = GetDlgItem(hdlg, IDC_WORD);
    HWND summary_hwnd = GetDlgItem(hdlg, IDC_SUMMARY);
    HWND answer_hwnd = GetDlgItem(hdlg, IDC_ANSWER);
    HWND your_answer_hwnd = GetDlgItem(hdlg, IDC_YOUR_ANSWER);

    switch (message) {
    case WM_INITDIALOG: {
        Edit_LimitText(answer_hwnd, MAX_ANSWER_CHARS);
        SetWindowSubclass(answer_hwnd, AkkadianEditControl, 0, NULL);
        state.reset();
        state.new_phrase(Akk::dict, engl);
        try {
            SetWindowTextW(word_hwnd, state.get_question(engl).c_str());
        }
        catch (PracticeError err) {
            MessageBoxW(hdlg, err.message().c_str(), L"Error", MB_ICONERROR | MB_OK);
            EndDialog(hdlg, IDABORT);
            return (INT_PTR)TRUE;
        }
        SetWindowTextW(summary_hwnd, state.get_summary(Akk::dict, engl, false).c_str());
        SetWindowTextW(answer_hwnd, L"");
        SetWindowTextW(your_answer_hwnd, L"");
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(w_param) == IDCANCEL) {
            EndDialog(hdlg, LOWORD(w_param));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(w_param) == IDOK) {
            std::wstring answer = get_input_txt(hdlg, IDC_ANSWER);
            PhraseAnswer ans_obj = state.accept_answer(answer, engl);
            SetWindowTextW(summary_hwnd, state.get_summary(Akk::dict, engl, ans_obj.correct, ans_obj.noun, ans_obj.adj).c_str());
            state.new_phrase(Akk::dict, engl);
            try {
                SetWindowTextW(word_hwnd, state.get_question(engl).c_str());
            }
            catch (PracticeError err) {
                MessageBoxW(hdlg, err.message().c_str(), L"Error", MB_ICONERROR | MB_OK);
                EndDialog(hdlg, IDABORT);
                return (INT_PTR)TRUE;
            }
            SetWindowTextW(answer_hwnd, L"");
            SetWindowTextW(your_answer_hwnd, (L"Your answer: " + answer).c_str());
            return (INT_PTR)TRUE;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}

static INT_PTR CALLBACK LookupDialog(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param, bool engl) {
    UNREFERENCED_PARAMETER(l_param);

    const static int CUTOFF = 4;
    const static int LIMIT = 15;

    const static wchar_t * default_txt = LR"(
Type a word into the box and press enter to search for definitions. Press the up arrow key
 in the box to cycle diacritical marks for the character to the left of the cursor.
    )";

    HWND results_hwnd = GetDlgItem(hdlg, IDC_LOOKUP_RESULTS);
    HWND input_hwnd = GetDlgItem(hdlg, IDC_LOOKUP_INPUT);

    switch (message) {
    case WM_INITDIALOG: {
        SetWindowSubclass(input_hwnd, AkkadianEditControl, 0, NULL);
        SetWindowTextW(results_hwnd, default_txt);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(w_param) == IDCANCEL) {
            EndDialog(hdlg, LOWORD(w_param));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(w_param) == IDOK) {
            std::wstring query = get_input_txt(hdlg, IDC_LOOKUP_INPUT);
            query = trim(query);
            std::vector<std::wstring> results = Akk::dict.search(query, LIMIT, CUTOFF, engl);

            if (results.size() == 0) {
                SetWindowTextW(results_hwnd, L"No results");
            }

            else {
                std::wstring result_summary;

                for (std::wstring& res : results) {
                    result_summary += engl ? Akk::dict.engl_summary(res) : Akk::dict.akk_summary(res);
                }

                result_summary = trim(result_summary);

                SetWindowTextW(results_hwnd, result_summary.c_str());
            }
            return (INT_PTR)TRUE;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK PracticeEnglishWords(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticeWordsDialog(hdlg, message, w_param, l_param, true);
}

INT_PTR CALLBACK PracticeAkkadianWords(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticeWordsDialog(hdlg, message, w_param, l_param, false);
}

INT_PTR CALLBACK PracticeEnglishPhrases(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticePhrasesDialog(hdlg, message, w_param, l_param, true);
}

INT_PTR CALLBACK PracticeAkkadianPhrases(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticePhrasesDialog(hdlg, message, w_param, l_param, false);
}

INT_PTR CALLBACK LookupEnglish(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return LookupDialog(hdlg, message, w_param, l_param, true);
}

INT_PTR CALLBACK LookupAkkadian(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return LookupDialog(hdlg, message, w_param, l_param, false);
}
