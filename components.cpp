#pragma once

#include <assert.h>
#include <vector>
#include <Windows.h>
#include <CommCtrl.h>

#include "common.h"
#include "components.h"

const static std::vector<std::vector<wchar_t>> char_cycle_map = {
    {L's', L'š', L'ṣ'},
    {L't', L'ṭ'},
    {L'a', L'ā', L'â'},
    {L'e', L'ē', L'ê'},
    {L'i', L'ī', L'î'},
    {L'u', L'ū', L'û'},
    {L'h', L'ẖ'}
};

static wchar_t next_char(wchar_t c) {
    for (size_t i = 0; i < char_cycle_map.size(); i++) {
        size_t len = char_cycle_map[i].size();

        for (size_t j = 0; j < len; j++) {
            if (char_cycle_map[i][j] == c) {
                return char_cycle_map[i][(j + 1) % len];
            }
        }
    }

    return c;
}

LRESULT CALLBACK AkkadianEditControl(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR u_id_subclass, DWORD_PTR dw_ref_data) {
    switch (msg) {
    case WM_KEYDOWN: {
        if (w_param == VK_UP) {
            int res = SendMessage(hwnd, EM_GETSEL, NULL, NULL);
            int caret = LOWORD(res);
            int end = HIWORD(res);

            if (caret == 0) {
                break;
            }

            wchar_t answer_buf[MAX_ANSWER_CHARS + 1];

            GetWindowTextW(hwnd, answer_buf, MAX_ANSWER_CHARS);
            int len = lstrlenW(answer_buf);

            assert(len <= MAX_ANSWER_CHARS);
            assert(caret <= len);
            answer_buf[len] = L'\0';

            wchar_t old_char = answer_buf[caret - 1];
            wchar_t new_char = next_char(old_char);
            answer_buf[caret - 1] = new_char;
            SetWindowTextW(hwnd, answer_buf);
            SendMessage(hwnd, EM_SETSEL, caret, end);
            return (INT_PTR)TRUE;
        }

        break;
    }
    }

    return DefSubclassProc(hwnd, msg, w_param, l_param);
}
