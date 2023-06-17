/**
 * Dialog callback functions and other related functions.
 *
 * Author: Joe Desmond - desmonji@bc.edu
 */
#pragma once
#include <windows.h>

INT_PTR CALLBACK PracticeEnglishWords(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param);

INT_PTR CALLBACK PracticeAkkadianWords(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param);

INT_PTR CALLBACK PracticeEnglishPhrases(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param);

INT_PTR CALLBACK PracticeAkkadianPhrases(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param);

INT_PTR CALLBACK LookupEnglish(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param);

INT_PTR CALLBACK LookupAkkadian(HWND hdlg, UINT message, WPARAM w_param, LPARAM l_param);
