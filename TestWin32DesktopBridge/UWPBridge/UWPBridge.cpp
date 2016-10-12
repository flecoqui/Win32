// UWPBridge.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <winstring.h>
#include <iostream>
#include <iomanip>
#include <roapi.h>
#include <windows.globalization.h>
#include <Windows.System.Profile.h>
#include <appmodel.h>
#include <windows.ui.notifications.h>
#include <wrl\client.h>
#include <wrl\implements.h>

#pragma comment(lib, "runtimeobject.lib")
using namespace std;
using namespace Microsoft::WRL;

using namespace ABI::Windows::Globalization;
using namespace ABI::Windows::System::Profile;

using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

extern "C" __declspec(dllexport) BOOL APIENTRY GetUWPVersion(unsigned int* pv1, unsigned int* pv2, unsigned int* pv3, unsigned int* pv4)
{
	bool bResult = FALSE;
	HRESULT hr;
	hr = ::RoInitialize(RO_INIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		HSTRING hClassName;
		LPWSTR pstring= L"Windows.System.Profile.AnalyticsInfo";
		UINT32 len = (UINT32) wcslen(pstring);

		hr = ::WindowsCreateString(pstring, len, &hClassName);
		if (SUCCEEDED(hr))
		{
			IAnalyticsInfoStatics* pInfo;
			hr = ::RoGetActivationFactory(hClassName, __uuidof(IAnalyticsInfoStatics), (void**)&pInfo);
			if (SUCCEEDED(hr))
			{
				IAnalyticsVersionInfo* pVersionInfo;
				hr = pInfo->get_VersionInfo(&pVersionInfo);
				if (SUCCEEDED(hr))
				{

					HSTRING hVersion;
					LPWSTR pversionstring = L"10";
					UINT32 lenversion = (UINT32) wcslen(pstring);

					hr = ::WindowsCreateString(pversionstring, lenversion, &hVersion);
					if (SUCCEEDED(hr))
					{
						hr = pVersionInfo->get_DeviceFamilyVersion(&hVersion);
						if (SUCCEEDED(hr))
						{
							UINT32 len = 0;
							PCWSTR p = ::WindowsGetStringRawBuffer(hVersion, NULL);
							unsigned long long v = wcstoull(p, NULL, 10);
							*pv1 = (v & 0xFFFF000000000000L) >> 48;
							*pv2 = (v & 0x0000FFFF00000000L) >> 32;
							*pv3 = (v & 0x00000000FFFF0000L) >> 16;
							*pv4 = (v & 0x000000000000FFFFL);
							bResult = TRUE;
						}
						::WindowsDeleteString(hVersion);
					}
					pVersionInfo->Release();
				}
				pInfo->Release();
			}
			::WindowsDeleteString(hClassName);
		}
		::RoUninitialize();
	}
	return bResult;
}
HRESULT NewSetNodeValueString(_In_ HSTRING inputString, _In_ IXmlNode *node, _In_ IXmlDocument *xml)
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

HRESULT SetNodeValueString( HSTRING inputString, IXmlNode *node, IXmlDocument *xml)
{
	IXmlText *pInputText;

	HRESULT hr = xml->CreateTextNode(inputString, &pInputText);
	if (SUCCEEDED(hr))
	{
		IXmlNode* pInputTextNode;

		hr = pInputText->QueryInterface(__uuidof(IXmlNode),(void**) &pInputTextNode);
		if (SUCCEEDED(hr))
		{
			IXmlNode* pAppendedChild;
			hr = node->AppendChild(pInputTextNode, &pAppendedChild);
			if (SUCCEEDED(hr))
			{
				pAppendedChild->Release();
			}
			pInputTextNode->Release();
		}
		pInputText->Release();
	}

	return hr;
}
HRESULT SetTextValues(LPCWSTR *textValues, UINT32 textValuesCount, UINT32 *textValuesLengths, IXmlDocument *pToastXml)
{
	HRESULT hr = textValues != nullptr && textValuesCount > 0 ? S_OK : E_INVALIDARG;
	if (SUCCEEDED(hr))
	{
		IXmlNodeList* pNodeList;
		HSTRING hText;
		LPWSTR pstring = L"text";
		UINT32 len = (UINT32)wcslen(pstring);

		hr = ::WindowsCreateString(pstring, len, &hText);
		if (SUCCEEDED(hr))
		{
			hr = pToastXml->GetElementsByTagName(hText, &pNodeList);
			if (SUCCEEDED(hr))
			{
				UINT32 nodeListLength;
				hr = pNodeList->get_Length(&nodeListLength);
				if (SUCCEEDED(hr))
				{
					hr = textValuesCount <= nodeListLength ? S_OK : E_INVALIDARG;
					if (SUCCEEDED(hr))
					{
						for (UINT32 i = 0; i < textValuesCount; i++)
						{
							IXmlNode* pTextNode;
							hr = pNodeList->Item(i, &pTextNode);
							if (SUCCEEDED(hr))
							{
								HSTRING hTextValue;
								hr = ::WindowsCreateString(textValues[i], textValuesLengths[i], &hTextValue);
								if (SUCCEEDED(hr))
								{
									hr = SetNodeValueString(hTextValue, pTextNode, pToastXml);
									::WindowsDeleteString(hTextValue);
								}
								pTextNode->Release();
							}
						}
					}
				}
				pNodeList->Release();
			}
			::WindowsDeleteString(hText);
		}
	}
	return hr;
}

HRESULT SetImageSrc( LPCWSTR imagePath, IXmlDocument *pToastXml)
{
	wchar_t imageSrc[MAX_PATH] = L"file:///";
	HRESULT hr = StringCchCat(imageSrc, ARRAYSIZE(imageSrc), imagePath);
	if (SUCCEEDED(hr))
	{
		IXmlNodeList* pNodeList;
		HSTRING hImage;
		LPWSTR pstring = L"image";
		UINT32 len = (UINT32)wcslen(pstring);

		hr = ::WindowsCreateString(pstring, len, &hImage);
		if (SUCCEEDED(hr))
		{
			hr = pToastXml->GetElementsByTagName(hImage, &pNodeList);
			if (SUCCEEDED(hr))
			{
				IXmlNode* pImageNode;
				hr = pNodeList->Item(0, &pImageNode);
				if (SUCCEEDED(hr))
				{
					IXmlNamedNodeMap* pAttributes;

					hr = pImageNode->get_Attributes(&pAttributes);
					if (SUCCEEDED(hr))
					{

						HSTRING hSrc;
						LPWSTR psrcstring = L"src";
						UINT32 srclen = (UINT32)wcslen(psrcstring);
						hr = ::WindowsCreateString(psrcstring, srclen, &hSrc);
						if (SUCCEEDED(hr))
						{
							IXmlNode* pSrcAttribute;
							hr = pAttributes->GetNamedItem(hSrc, &pSrcAttribute);
							if (SUCCEEDED(hr))
							{
								HSTRING  hImageSrc;
								hr = ::WindowsCreateString(imageSrc, (UINT32)wcslen(imageSrc), &hImageSrc);
								if (SUCCEEDED(hr))
								{
									hr = SetNodeValueString(hImageSrc, pSrcAttribute, pToastXml);
									::WindowsDeleteString(hImageSrc);
								}
								pSrcAttribute->Release();
							}
							::WindowsDeleteString(hSrc);
						}
						pAttributes->Release();
					}
					pImageNode->Release();
				}
				pNodeList->Release();
			}			
			::WindowsDeleteString(hImage);
		}
	}
	return hr;
}
typedef HRESULT(__stdcall * CallbackActivated)();

typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

class ToastEventHandler :
	public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
{
public:
	ToastEventHandler::ToastEventHandler(
		_In_ 	CallbackActivated hActivated) : HActivated(hActivated)
	{

	};
	~ToastEventHandler()
	{

	}

	// DesktopToastActivatedEventHandler 
	IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable* args)
	{
		if (HActivated != nullptr)
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
			if (HActivated != nullptr)
				return HActivated();
//			if (HDismissed != nullptr)
	//			return HDismissed(tdr);
		}
		return S_OK;
	}

	// DesktopToastFailedEventHandler
	IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e)
	{
//		if (HFailed != nullptr)
	//		return HFailed();
		if (HActivated != nullptr)
			return HActivated();
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
};

HRESULT ShowToast(IToastNotificationManagerStatics *pManager, IXmlDocument *pToastXml, LPCWSTR pAppId, HRESULT(__stdcall * callbackActivated)())
{
	IToastNotifier* pNotifier;
	HRESULT hr = 0;
	HSTRING hAppId;
	UINT32 len = (UINT32)wcslen(pAppId);
	hr = ::WindowsCreateString(pAppId, len, &hAppId);
	if (SUCCEEDED(hr))
	{
		hr = pManager->CreateToastNotifierWithId(hAppId, &pNotifier);
		if (SUCCEEDED(hr))
		{
			IToastNotificationFactory* pFactory;
			HSTRING hClassName;
			LPWSTR pstring = L"Windows.UI.Notifications.ToastNotification";
			UINT32 len = (UINT32)wcslen(pstring);

			hr = ::WindowsCreateString(pstring, len, &hClassName);
			if (SUCCEEDED(hr))
			{
				hr = ::RoGetActivationFactory(hClassName, __uuidof(IToastNotificationFactory),(void**)&pFactory);
				if (SUCCEEDED(hr))
				{
					IToastNotification* pToast;
					hr = pFactory->CreateToastNotification(pToastXml, &pToast);
					if (SUCCEEDED(hr))
					{
						// Register the event handlers
						
						EventRegistrationToken activatedToken;
						ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(callbackActivated));
						if (eventHandler != NULL)
						{
							hr = pToast->add_Activated(eventHandler.Get(), &activatedToken);
							if (SUCCEEDED(hr))
							{
								hr = pToast->add_Dismissed(eventHandler.Get(), &activatedToken);
								if (SUCCEEDED(hr))
								{
									hr = pToast->add_Failed(eventHandler.Get(), &activatedToken);
									if (SUCCEEDED(hr))
									{
										hr = pNotifier->Show(pToast);
									}
								}
							//	hr = pNotifier->Show(pToast);
							}
							//eventHandler->Release();
						}
						pToast->Release();
					}
					pFactory->Release();
				}
			}
		}
		::WindowsDeleteString(hAppId);
	}
	return hr;
}
extern "C" __declspec(dllexport) BOOL APIENTRY ShowUWPToast(LPCWSTR pLogo, LPCWSTR pTitle, LPCWSTR pMessage, LPCWSTR pAppId, HRESULT(__stdcall * callbackActivated)())
{
	HRESULT hr;
	hr = ::RoInitialize(RO_INIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		HSTRING hClassName;
		LPWSTR pstring = L"Windows.UI.Notifications.ToastNotificationManager";
		UINT32 len = (UINT32)wcslen(pstring);

		hr = ::WindowsCreateString(pstring, len, &hClassName);
		if (SUCCEEDED(hr))
		{
			IToastNotificationManagerStatics* pManager;
			hr = ::RoGetActivationFactory(hClassName, __uuidof(IToastNotificationManagerStatics), (void**)&pManager);
			if (SUCCEEDED(hr))
			{
				IXmlDocument* pToastXml;
				hr = pManager->GetTemplateContent(ToastTemplateType_ToastImageAndText04, &pToastXml);
				if (SUCCEEDED(hr))
				{
					hr = pLogo != nullptr ? S_OK : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
					if (SUCCEEDED(hr))
					{
						hr = SetImageSrc(pLogo, pToastXml);
						if (SUCCEEDED(hr))
						{
							LPCWSTR textValues[] = {
								pTitle,
								pMessage,
								L""
							};

							UINT32 textLengths[] = { (UINT32)wcslen(pTitle), (UINT32)wcslen(pMessage), 0 };

							hr = SetTextValues(textValues, 3, textLengths, pToastXml);
							if (SUCCEEDED(hr))
							{								
								hr = ShowToast(pManager,pToastXml, pAppId, callbackActivated);
							}
						}
					}
					pToastXml->Release();
				}
				pManager->Release();
			}
			::WindowsDeleteString(hClassName);
		}
		::RoUninitialize();
	}
	return TRUE;
}

extern "C" __declspec(dllexport)  BOOL APIENTRY InContainer()
{
	UINT32 length = 0;

	LONG rc = ::GetCurrentPackageFullName(&length, 0);
	if (rc != ERROR_INSUFFICIENT_BUFFER)
	{
		if (rc != APPMODEL_ERROR_NO_PACKAGE)
			return FALSE;
		return FALSE;
	}

	PWSTR fullName = (PWSTR)malloc(length * sizeof(*fullName));
	if (fullName == NULL)
	{
		return FALSE;
	}

	rc = GetCurrentPackageFullName(&length, fullName);
	if (rc != ERROR_SUCCESS)
	{
		return FALSE;
	}
	return TRUE;
}