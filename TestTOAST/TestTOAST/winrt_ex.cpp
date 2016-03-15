/*****************************************************************************
* winrt_ex.cpp : functions to winrt api from win32 applications
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
// winrt_ex.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "winrt_ex.h"
using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

using namespace ABI::Windows::UI::Notifications;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

const wchar_t AppId[] = L"vlc.toasts";

class ToastEventHandler :
	public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
{
public:
	ToastEventHandler::ToastEventHandler(
		_In_ 	CallbackActivated hActivated,
		_In_ 	CallbackDismissed hDismissed,
		_In_ 	CallbackFailed hFailed) : HActivated(hActivated), HDismissed(hDismissed), HFailed(hFailed)
	{

	};
	~ToastEventHandler()
	{

	}

	// DesktopToastActivatedEventHandler 
	IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable* args)
	{
		if (HActivated!= nullptr)
			return HActivated();
		
		return S_OK;
	}

	// DesktopToastDismissedEventHandler
	IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IToastDismissedEventArgs* e)
	{
		ToastDismissalReason tdr;
		HRESULT hr = e->get_Reason(&tdr);
		if (SUCCEEDED(hr))
		{
			if (HDismissed != nullptr)
				return HDismissed(tdr);
		}
		return S_OK;
	}

	// DesktopToastFailedEventHandler
	IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e)
	{
		if (HFailed != nullptr)
			return HFailed();
		return S_OK;
	}


	// IUnknown
	IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref); }

	IFACEMETHODIMP_(ULONG) Release() {
		ULONG l = InterlockedDecrement(&_ref);
		if (l == 0) delete this;
		return l;
	}

	IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv) {
		if (IsEqualIID(riid, IID_IUnknown))
			*ppv = static_cast<IUnknown*>(static_cast<DesktopToastActivatedEventHandler*>(this));
		else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler)))
			*ppv = static_cast<DesktopToastActivatedEventHandler*>(this);
		else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler)))
			*ppv = static_cast<DesktopToastDismissedEventHandler*>(this);
		else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler)))
			*ppv = static_cast<DesktopToastFailedEventHandler*>(this);
		else *ppv = nullptr;

		if (*ppv) {
			reinterpret_cast<IUnknown*>(*ppv)->AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

private:
	ULONG _ref;
	CallbackActivated HActivated;
	CallbackDismissed HDismissed;
	CallbackFailed HFailed;
};

class StringReferenceWrapper
{
public:

	// Constructor which takes an existing string buffer and its length as the parameters.
	// It fills an HSTRING_HEADER struct with the parameter.      
	// Warning: The caller must ensure the lifetime of the buffer outlives this      
	// object as it does not make a copy of the wide string memory.      

	StringReferenceWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length) throw()
	{
		HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);

		if (FAILED(hr))
		{
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
		}
	}

	~StringReferenceWrapper()
	{
		WindowsDeleteString(_hstring);
	}

	template <size_t N>
	StringReferenceWrapper(_In_reads_(N) wchar_t const (&stringRef)[N]) throw()
	{
		UINT32 length = N - 1;
		HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);

		if (FAILED(hr))
		{
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
		}
	}

	template <size_t _>
	StringReferenceWrapper(_In_reads_(_) wchar_t(&stringRef)[_]) throw()
	{
		UINT32 length;
		HRESULT hr = SizeTToUInt32(wcslen(stringRef), &length);

		if (FAILED(hr))
		{
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
		}

		WindowsCreateStringReference(stringRef, length, &_header, &_hstring);
	}

	HSTRING Get() const throw()
	{
		return _hstring;
	}


private:
	HSTRING             _hstring;
	HSTRING_HEADER      _header;
};


HRESULT SetNodeValueString(_In_ HSTRING inputString, _In_ IXmlNode *node, _In_ IXmlDocument *xml)
{
	ComPtr<IXmlText> inputText;

	HRESULT hr = xml->CreateTextNode(inputString, &inputText);
	if (SUCCEEDED(hr))
	{
		ComPtr<IXmlNode> inputTextNode;

		hr = inputText.As(&inputTextNode);
		if (SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> pAppendedChild;
			hr = node->AppendChild(inputTextNode.Get(), &pAppendedChild);
		}
	}

	return hr;
}
// Set the values of each of the text nodes
HRESULT SetTextValues(_In_reads_(textValuesCount) wchar_t **textValues, _In_ UINT32 textValuesCount, _In_reads_(textValuesCount) UINT32 *textValuesLengths, _In_ IXmlDocument *toastXml)
{
	HRESULT hr = textValues != nullptr && textValuesCount > 0 ? S_OK : E_INVALIDARG;
	if (SUCCEEDED(hr))
	{
		ComPtr<IXmlNodeList> nodeList;
		hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
		if (SUCCEEDED(hr))
		{
			UINT32 nodeListLength;
			hr = nodeList->get_Length(&nodeListLength);
			if (SUCCEEDED(hr))
			{
				hr = textValuesCount <= nodeListLength ? S_OK : E_INVALIDARG;
				if (SUCCEEDED(hr))
				{
					for (UINT32 i = 0; i < textValuesCount; i++)
					{
						ComPtr<IXmlNode> textNode;
						hr = nodeList->Item(i, &textNode);
						if (SUCCEEDED(hr))
						{
							hr = SetNodeValueString(StringReferenceWrapper(textValues[i], textValuesLengths[i]).Get(), textNode.Get(), toastXml);
						}
					}
				}
			}
		}
	}
	return hr;
}



// Set the value of the "src" attribute of the "image" node
HRESULT SetImageSrc(_In_z_ wchar_t *imagePath, _In_ IXmlDocument *toastXml)
{
	wchar_t imageSrc[MAX_PATH] = L"file:///";
	HRESULT hr = StringCchCat(imageSrc, ARRAYSIZE(imageSrc), imagePath);
	if (SUCCEEDED(hr))
	{
		ComPtr<IXmlNodeList> nodeList;
		hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
		if (SUCCEEDED(hr))
		{
			ComPtr<IXmlNode> imageNode;
			hr = nodeList->Item(0, &imageNode);
			if (SUCCEEDED(hr))
			{
				ComPtr<IXmlNamedNodeMap> attributes;

				hr = imageNode->get_Attributes(&attributes);
				if (SUCCEEDED(hr))
				{
					ComPtr<IXmlNode> srcAttribute;

					hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
					if (SUCCEEDED(hr))
					{
						hr = SetNodeValueString(StringReferenceWrapper(imageSrc).Get(), srcAttribute.Get(), toastXml);
					}
				}
			}
		}
	}
	return hr;
}
// Create the toast XML from a template
HRESULT CreateToastXml(_In_ IToastNotificationManagerStatics *toastManager, wchar_t *PathImage, wchar_t *Title, wchar_t *Message, _Outptr_ IXmlDocument** inputXml)
{
	// Retrieve the template XML
	HRESULT hr = toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText04, inputXml);
	if (SUCCEEDED(hr))
	{
		//      wchar_t *imagePath = _wfullpath(nullptr, L"toastImageAndText.png", MAX_PATH);

		hr = PathImage != nullptr ? S_OK : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		if (SUCCEEDED(hr))
		{
			hr = SetImageSrc(PathImage, *inputXml);
			if (SUCCEEDED(hr))
			{
				wchar_t* textValues[] = {
					Title,
					Message,
					L""
				};

				UINT32 textLengths[] = { wcslen(Title), wcslen(Message), 0 };

				hr = SetTextValues(textValues, 3, textLengths, *inputXml);
			}
		}
	}
	return hr;
}
// Create and display the toast
HRESULT CreateToast(_In_ IToastNotificationManagerStatics *toastManager, _In_ IXmlDocument *xml, 
	CallbackActivated hActivated,
	CallbackDismissed hDismissed,
	CallbackFailed hFailed)
{
	ComPtr<IToastNotifier> notifier;
	HRESULT hr = 0;
	
	hr = toastManager->CreateToastNotifierWithId(StringReferenceWrapper(AppId, wcslen(AppId)).Get(), &notifier);
	if (SUCCEEDED(hr))
	{
		ComPtr<IToastNotificationFactory> factory;
		hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &factory);
		if (SUCCEEDED(hr))
		{
			ComPtr<IToastNotification> toast;
			hr = factory->CreateToastNotification(xml, &toast);
			if (SUCCEEDED(hr))
			{
				// Register the event handlers
				EventRegistrationToken activatedToken, dismissedToken, failedToken;
			    ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(hActivated, hDismissed, hFailed));

				hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
				if (SUCCEEDED(hr))
				{
					hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
					if (SUCCEEDED(hr))
					{
						hr = toast->add_Failed(eventHandler.Get(), &failedToken);
						if (SUCCEEDED(hr))
						{
							hr = notifier->Show(toast.Get());
						}
					}
				}
			}
		}
	}
	
	return hr;
}
int winrt_displaytoast(wchar_t *PathImage, wchar_t *Title, wchar_t *Message, 
	CallbackActivated hActivated,
	CallbackDismissed hDismissed,
	CallbackFailed hFailed)
	
{
	ComPtr<IToastNotificationManagerStatics> toastStatics;
	HRESULT hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &toastStatics);

	if (SUCCEEDED(hr))
	{
		ComPtr<IXmlDocument> toastXml;
		hr = CreateToastXml(toastStatics.Get(), PathImage, Title, Message, &toastXml);
		if (SUCCEEDED(hr))
		{
			hr = CreateToast(toastStatics.Get(), toastXml.Get(), hActivated, hDismissed, hFailed);
		}
	}
	return hr;
}
