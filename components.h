#pragma once

#include <Windows.h>

/**
 * An edit control. Pressing the up arrow key while focused will cycle diacritical marks on the character
 * to the left of the caret. Possible diacritical marks are those used for the letters in the alphabet
 * given in Huehnergard's book. This callback is meant to be used by subclassing an existing edit control and
 * passing this as the event handler.
 */
LRESULT CALLBACK AkkadianEditControl(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR u_id_subclass, DWORD_PTR dw_ref_data);
