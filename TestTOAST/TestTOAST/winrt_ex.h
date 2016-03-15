/*****************************************************************************
* winrt_ex.h : functions to winrt api from win32 applications
*****************************************************************************
* Copyright (C) 2008,2016 VLC authors and VideoLAN
*
* Authors: 
*          Fred Le Coqui <flecoqui@gmail.com>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program; if not, write to the Free Software Foundation,
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA    02111, USA.
*****************************************************************************/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WINRT_EX_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WINRT_EX_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WINRT_EX_EXPORTS
#define WINRT_EX_API __declspec(dllexport)
#else
#define WINRT_EX_API __declspec(dllimport)
#endif


using namespace ABI::Windows::UI::Notifications;
typedef HRESULT(__stdcall * CallbackActivated)();
typedef HRESULT(__stdcall * CallbackDismissed)(ToastDismissalReason tdr);
typedef HRESULT(__stdcall * CallbackFailed)();


WINRT_EX_API int winrt_displaytoast(wchar_t *PathImage, wchar_t *Title, wchar_t *Message , 
	CallbackActivated hActivated,
	CallbackDismissed hDismissed,
	CallbackFailed hFailed
	);

