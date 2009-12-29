/*
 Copyright (C) 2009 Moutaz Haq <cefarix@gmail.com>
 This file is released under the Code Project Open License <http://www.codeproject.com/info/cpol10.aspx>
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <tchar.h>

// Our main CLSID in string format
#define CLSID_IEPlugin_Str _T("{3543619C-D563-43f7-95EA-4DA7E1CC396A}")
extern const CLSID CLSID_IEPlugin;
extern volatile LONG DllRefCount;
extern HINSTANCE hInstance;

#endif // __COMMON_H__
