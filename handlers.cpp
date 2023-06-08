#include <algorithm>
#include <assert.h>
#include <iomanip>
#include <Windows.h>
#include <CommCtrl.h>
#include <sstream>
#include <string>
#include <windowsx.h>

#include "common.h"
#include "components.h"
#include "errors.h"
#include "handlers.h"
#include "resource.h"

static std::wstring get_input_txt(HWND hdlg, int res_id) {
    wchar_t buf[MAX_ANSWER_CHARS + 1];
    int chars_read = GetDlgItemTextW(hdlg, res_id, buf, MAX_ANSWER_CHARS);
    buf[chars_read] = '\0';

    return std::wstring(buf);
}

static std::wstring get_word_class_str(std::vector<WordClass>& classes) {
    std::wstring out;

    for (size_t i = 0; i < classes.size() - 1; i++) {
        out += WORD_CLASSES[classes[i]] + L", ";
    }
    
    if (classes.size() != 0) {
        out += WORD_CLASSES[classes[classes.size() - 1]];
    }

    return out;
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

void PracticeState::reset() {
    correct = 0;
    total = 0;
    word = L"";
}

void PracticeState::new_word(Dictionary& dict, bool engl) {
    std::pair<std::wstring, DictEntry> item = engl ? dict.random_engl() : dict.random_akk();
    word = item.first;
    entry = item.second;
}

bool PracticeState::accept_answer(std::wstring& answer) {
    bool retval = false;

    for (std::wstring defn : entry.defns) {
        if (defn == answer) {
            correct++;
            retval = true;
        }
    }
    total++;

    return retval;
}

std::wstring PracticeState::get_summary(Dictionary& dict, bool engl, bool wasCorrect, std::wstring answer = L"") {
    if (total == 0) {
        return L"0/0";
    }

    DictEntry entry = this->entry;

    // Find the Akkadian word's definition
    if (engl && wasCorrect) {
        const std::vector<DictEntry>* entries = *dict.get_akk(answer);

        for (const DictEntry& e : *entries) {
            if (e.grammar_kind == entry.grammar_kind && e.word_types == entry.word_types) {
                entry = e;
                this->word = answer;
                break;
            }
        }

        // This is unreachable because at this point we know that the answer exists in the dictionary
        __assume(false);
    }

    std::wostringstream score;
    score << std::fixed << std::setprecision(2) << ((correct / (double)total) * 100);

    std::wstring out = std::to_wstring(correct) + L"/" + std::to_wstring(total) 
        + L" (" + score.str() + L"%) ";

    out += get_question() += L":\n";

    for (size_t i = 0; i < entry.defns.size() - 1; i++) {
        std::wstring defn = entry.defns[i];
        out += defn + L", ";
    }

    out += entry.defns[entry.defns.size() - 1];

    return out;
}

std::wstring PracticeState::get_question() {
    std::wstring attrs = GRAMMAR_KINDS[entry.grammar_kind];

    if (entry.word_types.size() != 0) {
        attrs += L"; " + get_word_class_str(entry.word_types);
    }

    bool is_pret_of = std::find_if(entry.relations.begin(), entry.relations.end(), [](const WordRelation& w) {
        return w.kind == WordRelationKind::PreteriteOf;
    }) != entry.relations.end();

    if (is_pret_of) {
        attrs += L", pret";
    }

    return word + L" (" + attrs + L")";
}

static INT_PTR CALLBACK PracticeDialog(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param, bool engl) {
    UNREFERENCED_PARAMETER(l_param);

    static PracticeState state;

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

INT_PTR CALLBACK PracticeEnglish(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticeDialog(hdlg, message, w_param, l_param, true);
}

INT_PTR CALLBACK PracticeAkkadian(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticeDialog(hdlg, message, w_param, l_param, false);
}

INT_PTR CALLBACK LookupEnglish(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return LookupDialog(hdlg, message, w_param, l_param, true);
}

INT_PTR CALLBACK LookupAkkadian(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return LookupDialog(hdlg, message, w_param, l_param, false);
}
