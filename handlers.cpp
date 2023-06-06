#include <assert.h>
#include <iomanip>
#include <Windows.h>
#include <CommCtrl.h>
#include <sstream>
#include <string>
#include <windowsx.h>

#include "common.h"
#include "components.h"
#include "handlers.h"
#include "resource.h"

static std::wstring get_input_txt(HWND hDlg, int resId) {
    wchar_t buf[MAX_ANSWER_CHARS + 1];
    int chars_read = GetDlgItemTextW(hDlg, resId, buf, MAX_ANSWER_CHARS);
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

    if (engl && wasCorrect) {
        std::vector<DictEntry> entries = dict.akk_to_engl[answer];

        for (DictEntry e : entries) {
            if (e.grammar_kind == entry.grammar_kind && e.word_types == entry.word_types) {
                entry = e;
                this->word = answer;
                goto found;
            }
        }

        throw 1;
    }
found:

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

INT_PTR CALLBACK PracticeEnglish(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticeDialog(hdlg, message, w_param, l_param, true);
}

INT_PTR CALLBACK PracticeAkkadian(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param) {
    return PracticeDialog(hdlg, message, w_param, l_param, false);
}
