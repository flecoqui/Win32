#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <sal.h>
#include <Psapi.h>
#include <strsafe.h>
#include <ObjBase.h>
#include <ShObjIdl.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>
#include <intsafe.h>
#include <guiddef.h>
#include <ppltasks.h>
#include <roapi.h>
#include <wrl\client.h>
#include <wrl\event.h>
#include <wrl\implements.h>
#include <windows.ui.notifications.h>
#include <windows.Foundation.h>
#include <windows.storage.h>
#include <windows.System.Threading.h>

using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Details;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::FileProperties;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Foundation;
using namespace concurrency;

using namespace ABI::Windows::UI::Notifications;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

typedef struct _WIN32_FIND_RESULT_HEADER {
	DWORD dwItemsFound;
	DWORD dwCurrentItem;
	WIN32_FIND_DATAW **lpResult;
}WIN32_FIND_RESULT_HEADER;


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
	PCWSTR GetString()
	{
		UINT32 len;
		return WindowsGetStringRawBuffer(_hstring, &len); 
	}


private:
	HSTRING             _hstring;
	HSTRING_HEADER      _header;
};


HANDLE
WINAPI
NewFirstFind(
	_In_ LPCWSTR lpFileName,
	_Out_ WIN32_FIND_DATA* pData
)
{
	if (lpFileName != nullptr)
	{
		HRESULT hr = RoInitialize((RO_INIT_TYPE)RO_INIT_MULTITHREADED);
		if (FAILED(hr))
		{
			return INVALID_HANDLE_VALUE;
		}
		// Create an event that is set after the timer callback completes. We later use this event to wait for the timer to complete. 
		// This event is for demonstration only in a console app. In most apps, you typically don't wait for async operations to complete.
		Event timerCompleted(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, WRITE_OWNER | EVENT_ALL_ACCESS));
		hr = timerCompleted.IsValid() ? S_OK : HRESULT_FROM_WIN32(GetLastError());
		if (SUCCEEDED(hr))
		{

			ComPtr<IStorageFolderStatics> storageFolderStatics;
			HRESULT hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_Storage_StorageFolder).Get(), &storageFolderStatics);
			if (SUCCEEDED(hr))
			{
				__FIAsyncOperation_1_Windows__CStorage__CStorageFolder *pAsync;
				hr = storageFolderStatics.Get()->GetFolderFromPathAsync(StringReferenceWrapper(lpFileName, (UINT32)wcslen(lpFileName)).Get(), &pAsync);
				if (SUCCEEDED(hr))
				{
					WIN32_FIND_RESULT_HEADER *pHeader = nullptr;
					hr = pAsync->put_Completed(Callback<IAsyncOperationCompletedHandler<ABI::Windows::Storage::StorageFolder*>	>
						([&timerCompleted,&pHeader](__FIAsyncOperation_1_Windows__CStorage__CStorageFolder* pHandler, ABI::Windows::Foundation::AsyncStatus status)
					{
						if (status == ABI::Windows::Foundation::AsyncStatus::Completed)
						{
							ABI::Windows::Storage::IStorageFolder *pFolder;
							HRESULT hr = pHandler->GetResults(&pFolder);
							if (SUCCEEDED(hr))
							{
								__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CIStorageItem *pAsyncItem;
								hr = pFolder->GetItemsAsyncOverloadDefaultStartAndCount(&pAsyncItem);
								if (SUCCEEDED(hr))
								{
									hr = pAsyncItem->put_Completed(
										Callback<IAsyncOperationCompletedHandler<ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Storage::IStorageItem*>*>>
										([&timerCompleted,&pHeader](__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CIStorageItem* pHandler, ABI::Windows::Foundation::AsyncStatus status)
									{
										if (status == ABI::Windows::Foundation::AsyncStatus::Completed)
										{
											ABI::Windows::Foundation::Collections::IVectorView< ABI::Windows::Storage::IStorageItem* >* pItems;
											HRESULT hr = pHandler->GetResults(&pItems);
											if (SUCCEEDED(hr))
											{

												unsigned int Count;
												hr = pItems->get_Size(&Count);
												if (SUCCEEDED(hr))
												{
													if (Count > 0)
													{
														pHeader = (WIN32_FIND_RESULT_HEADER*)malloc(sizeof(WIN32_FIND_RESULT_HEADER));
														if (pHeader != nullptr)
														{
															pHeader->dwItemsFound = Count;
															pHeader->dwCurrentItem = 0;
															pHeader->lpResult = (WIN32_FIND_DATAW**) malloc(Count * sizeof (WIN32_FIND_DATAW*));
															for (DWORD i = 0;i < Count;i++)
															{
																pHeader->lpResult[i] = (WIN32_FIND_DATAW*)malloc(sizeof(WIN32_FIND_DATAW));

															}
														}
													}
													for (unsigned int i = 0; i < Count;i++)
													{
														ABI::Windows::Storage::IStorageItem* pItem;

														hr = pItems->GetAt(i, &pItem);
														if (SUCCEEDED(hr))
														{
															HSTRING hs;
															//hr = pItem->get_Name(&hs);
															//if (SUCCEEDED(hr))
															//{
															//	UINT32 Len;
															//	PCWSTR p = ::WindowsGetStringRawBuffer(hs, &Len);
															//	if (p != nullptr)
															//		printf("File name: %ls \r\n", (LPCWSTR)p);
															//	StringCbCopyNW(((WIN32_FIND_DATAW*)*(pHeader->lpResult))[i].cAlternateFileName, 14*sizeof(WCHAR), p, Len * sizeof(WCHAR));
															//	::WindowsDeleteString(hs);
															//}
															hr = pItem->get_Path(&hs);
															if (SUCCEEDED(hr))
															{
																UINT32 Len;
																PCWSTR p = ::WindowsGetStringRawBuffer(hs, &Len);
																if (p != nullptr)
																	printf("Path: %ls \r\n", (LPCWSTR)p);
																StringCbCopyNW(pHeader->lpResult[i]->cFileName, (MAX_PATH -1) * sizeof(WCHAR), p, (Len) * sizeof(WCHAR));
																::WindowsDeleteString(hs);
															}
															ABI::Windows::Storage::FileAttributes Attr;
															hr = pItem->get_Attributes(&Attr);
															if (SUCCEEDED(hr))
															{
																if ((Attr & ABI::Windows::Storage::FileAttributes::FileAttributes_Directory) == ABI::Windows::Storage::FileAttributes::FileAttributes_Directory)
																	pHeader->lpResult[i]->dwFileAttributes = ABI::Windows::Storage::FileAttributes::FileAttributes_Directory;
																else
																	pHeader->lpResult[i]->dwFileAttributes = 0;

															}
														}
													}
												}

											}
										}
										// Set the completion event and return.
										SetEvent(timerCompleted.Get());
										return S_OK;
									}).Get());
									if (SUCCEEDED(hr))
										return S_OK;
								}
							}
						}
						// Set the completion event and return.
						SetEvent(timerCompleted.Get());
						return S_OK;
					}).Get());
					if (SUCCEEDED(hr))
					{
						// Wait for the timer to complete.
						WaitForSingleObjectEx(timerCompleted.Get(), INFINITE, FALSE);
						RoUninitialize();
						if (pData != nullptr)
						{
							DWORD dwCur = pHeader->dwCurrentItem;
							
							pData->dwFileAttributes = pHeader->lpResult[dwCur]->dwFileAttributes;
							for(DWORD i = 0; i < MAX_PATH; i++)
								pData->cFileName[i] = pHeader->lpResult[dwCur]->cFileName[i];
							for (DWORD i = 0; i < 14; i++)
								pData->cAlternateFileName[i] = 0;
							pHeader->dwCurrentItem++;
						}
						return (HANDLE) pHeader;
					}
				}
			}
		}
		RoUninitialize();
	};
	return INVALID_HANDLE_VALUE;
}




BOOL
WINAPI
NewFindNextFile(
	_In_ HANDLE hFindFile,
	_Out_ LPWIN32_FIND_DATAW pData
)
{
	if ((hFindFile != nullptr) &&
		(pData != nullptr))
	{
		WIN32_FIND_RESULT_HEADER *pHeader = (WIN32_FIND_RESULT_HEADER *)hFindFile;

		if (pHeader->dwCurrentItem < pHeader->dwItemsFound)
		{
			DWORD dwCur = pHeader->dwCurrentItem;
			pData->dwFileAttributes = pHeader->lpResult[dwCur]->dwFileAttributes;
			for (DWORD i = 0; i < MAX_PATH; i++)
				pData->cFileName[i] = pHeader->lpResult[dwCur]->cFileName[i];
			for (DWORD i = 0; i < 14; i++)
				pData->cAlternateFileName[i] = 0;
			pHeader->dwCurrentItem++;
			return TRUE;
		}
	}
	return FALSE;
}
BOOL
WINAPI
NewFindClose(
	_Inout_ HANDLE hFindFile
)
{
	if (hFindFile != nullptr)
	{
		WIN32_FIND_RESULT_HEADER *pHeader = (WIN32_FIND_RESULT_HEADER *)hFindFile;
		for (DWORD i = 0;i < pHeader->dwItemsFound;i++)
		{
			free(pHeader->lpResult[i]);
		}
		free(pHeader->lpResult);
		free(pHeader);
		return TRUE;
	}
	return FALSE;
}