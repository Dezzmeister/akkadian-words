/**
 * Custom dialog component(s).
 * 
 * Author: Joe Desmond - desmonji@bc.edu
 */
#pragma once

#include <Windows.h>

/**
 * An edit control for Akkadian input. Pressing the up arrow key while focused will cycle diacritical marks on the character
 * to the left of the caret. Possible diacritical marks are those used for the letters in the alphabet
 * given in Huehnergard's book. This callback is meant to be used by subclassing an existing edit control and
 * passing this function as the callback. "Subclassing" here is a Win32 feature: 
 * https://learn.microsoft.com/en-us/windows/win32/controls/subclassing-overview.
 */
LRESULT CALLBACK AkkadianEditControl(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR u_id_subclass, DWORD_PTR dw_ref_data);
