//------------------------------------------------------------------------------
// File: Graph.cppIID_IScanningTuner
//
// Desc: Sample code for BDA graph building.
//
// Copyright (c) 2000-2002, Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "pch.h"
#include "graphwinrt.h"
#include "graph.h"
#include "strsafe.h"

#define REGISTER_FILTERGRAPH 1
///////////////////////////////////////////////////////////////////////////////////
static
const
IID IID_ISampleGrabberCB = { 0x0579154A, 0x2B53, 0x4994,{ 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };

///////////////////////////////////////////////////////////////////////////////////

static
const
IID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce,{ 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3,{ 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_NullRenderer = { 0xC1F400A4, 0x3F08, 0x11d3,{ 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_VideoEffects1Category = { 0xcc7bfb42, 0xf175, 0x11d1,{ 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_VideoEffects2Category = { 0xcc7bfb43, 0xf175, 0x11d1,{ 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_AudioEffects1Category = { 0xcc7bfb44, 0xf175, 0x11d1,{ 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_AudioEffects2Category = { 0xcc7bfb45, 0xf175, 0x11d1,{ 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

///////////////////////////////////////////////////////////////////////////////////
// WARNING FLECOQUI
const CLSID CLSID_DVBTLocator = { 0x9cd64701, 0xbdf3, 0x4d14,{ 0x8e, 0x03, 0xf1, 0x29, 0x83, 0xd8, 0x66, 0x64 } };

//const CLSID CLSID_DVBTNetworkProvider = { 0x216c62df, 0x6d7f, 0x4e9a,{ 0x85, 0x71, 0x5, 0xf1, 0x4e, 0xdb, 0x76, 0x6a } };

const CLSID CLSID_DVBTuneRequest = { 0x15d6504a, 0x5494, 0x499c,{ 0x88, 0x6c, 0x97, 0x3c, 0x9e, 0x53, 0xb9, 0xf1 } };

const CLSID CLSID_DVBTuningSpace = { 0xc6b14b32, 0x76aa, 0x4a86,{ 0xa7, 0xac, 0x5c, 0x79, 0xaa, 0xf5, 0x8d, 0xa7 } };


const IID IID_IDVBTLocator = { 0x8664da16, 0xdda2, 0x42ac,{ 0x92, 0x6a, 0xc1, 0x8f, 0x91, 0x27, 0xc3, 0x02 } };

const IID IID_IDVBTuneRequest = { 0x0d6f567e, 0xa636, 0x42bb,{ 0x83, 0xba, 0xce, 0x4c, 0x17, 0x04, 0xaf, 0xa2 } };

const IID IID_IDVBTuningSpace = { 0xada0b268, 0x3b19, 0x4e5b,{ 0xac, 0xc4, 0x49, 0xf8, 0x52, 0xbe, 0x13, 0xba } };

const IID IID_IDVBTuningSpace2 = { 0x843188b4, 0xce62, 0x43db,{ 0x96, 0x6b, 0x81, 0x45, 0xa0, 0x94, 0xe0, 0x40 } };
const IID IID_IScanningTuner = { 0x1dfd0a5c, 0x0284, 0x11d3, {0x9d, 0x8e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x80 } };

/*****************************************************************************
* BDAOutput
*****************************************************************************/
BDAOutput::BDAOutput() :
	p_first(NULL), pp_next(&p_first)
{
	InitializeCriticalSection(&lock);
	wait = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
}

BDAOutput::~BDAOutput()
{
	Empty();
	DeleteCriticalSection(&lock);
	CloseHandle(wait);
}
static inline void block_ChainLastAppend(BLOCK ***ppp_last, BLOCK *p_block)
{
	BLOCK *p_last = p_block;

	**ppp_last = p_block;

	while (p_last->p_next) p_last = p_last->p_next;
	*ppp_last = &p_last->p_next;
}
void BDAOutput::Push(BLOCK *p_block)
{
	MutexLocker l(&lock);

	block_ChainLastAppend(&pp_next, p_block);

	if (wait == NULL)
		return;
	ReleaseSemaphore(wait, 1, NULL);
}
void block_Init(BLOCK *b, void *buf, size_t size)
{
	/* Fill all fields to their default */
	b->p_next = NULL;
	b->p_buffer = (BYTE*) buf;
	b->i_buffer = size;
	b->p_start = (BYTE*) buf;
	b->i_size = size;
	b->i_flags = 0;
	b->i_nb_samples = 0;
/*	b->i_pts =
		b->i_dts = VLC_TS_INVALID;
	b->i_length = 0;
	*/
	b->pf_release = NULL;
}

static void block_generic_Release(BLOCK *block)
{
	/* That is always true for blocks allocated with block_Alloc(). */
	//assert(block->p_start == (unsigned char *)(block + 1));
	//block_Invalidate(block);
	free(block);
}
/*libavcodec AVX optimizations require at least 32 - bytes. */
#define BLOCK_ALIGN        32

/** Initial reserved header and footer size. */
#define BLOCK_PADDING      32
BLOCK *block_Alloc(size_t size)
{
	/* 2 * BLOCK_PADDING: pre + post padding */
	const size_t alloc = sizeof(BLOCK) + BLOCK_ALIGN + (2 * BLOCK_PADDING)
		+ size;
	if (alloc <= size)
		return NULL;

	BLOCK *b = (BLOCK*) malloc(alloc);
	if (b == NULL)
		return NULL;

	block_Init(b, b + 1, alloc - sizeof(*b));
//	static_assert ((BLOCK_PADDING % BLOCK_ALIGN) == 0,
//		"BLOCK_PADDING must be a multiple of BLOCK_ALIGN");
	b->p_buffer += BLOCK_PADDING + BLOCK_ALIGN - 1;
	b->p_buffer = (BYTE *)(((uintptr_t)b->p_buffer) & ~(BLOCK_ALIGN - 1));
	b->i_buffer = size;
	b->pf_release = block_generic_Release;
	return b;
}
static inline void block_Release(BLOCK *p_block)
{
	p_block->pf_release(p_block);
}
static inline void block_ChainRelease(BLOCK *p_block)
{
	while (p_block)
	{
		BLOCK *p_next = p_block->p_next;
		block_Release(p_block);
		p_block = p_next;
	}
}

LONG BDAOutput::Pop(void *buf, ULONG len)
{
	MutexLocker l(&lock);
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	/*
	mtime_t i_deadline = mdate() + 250 * 1000;
	while (!p_first)
	{
		if (vlc_cond_timedwait(&wait, &lock, i_deadline))
			return -1;
	}
	*/
	if (p_first == NULL)
		return 0;
	size_t i_index = 0;
	while (i_index < len)
	{
		size_t i_copy = min(len - i_index, p_first->i_buffer);
		memcpy((BYTE *)buf + i_index, p_first->p_buffer, i_copy);

		i_index += i_copy;

		p_first->p_buffer += i_copy;
		p_first->i_buffer -= i_copy;

		if (p_first->i_buffer <= 0)
		{
			BLOCK *p_next = p_first->p_next;
			block_Release(p_first);

			p_first = p_next;
			if (!p_first)
			{
				pp_next = &p_first;
				break;
			}
		}
	}
	return i_index;
}

void BDAOutput::Empty()
{
	MutexLocker l(&lock);

	if (p_first)
		block_ChainRelease(p_first);
	p_first = NULL;
	pp_next = &p_first;
}

// 
// NOTE: In this sample, text strings are hard-coded for 
// simplicity and for readability.  For product code, you should
// use string tables and LoadString().
//

//
// An application can advertise the existence of its filter graph
// by registering the graph with a global Running Object Table (ROT).
// The GraphEdit application can detect and remotely view the running
// filter graph, allowing you to 'spy' on the graph with GraphEdit.
//
// To enable registration in this sample, define REGISTER_FILTERGRAPH.
//


// We use channel 46 internally for testing.  Change this constant to any value.
#define DEFAULT_PHYSICAL_CHANNEL    46L


// Constructor, initializes member variables
// and calls InitializeGraphBuilder
CBDAFilterGraph::CBDAFilterGraph() :
    m_fGraphBuilt(FALSE),
    m_fGraphRunning(FALSE),
    m_lMajorChannel(-1), 
    m_lMinorChannel(-1),
    m_lPhysicalChannel(DEFAULT_PHYSICAL_CHANNEL),
	m_frequency(586000000),
	m_fec_hp(-1),
	m_fec_lp(-1),
	m_bandwidth(8000000),
	m_transmission(-1),
	m_guard(-1),
	m_hierarchy(-1),
    m_dwGraphRegister (0),
	p_access(NULL),
	guid_network_type(GUID_NULL),
	l_tuner_used(-1),
	systems(0),
	d_graph_register(0)
// FLECOQUI
//	output(NULL)
{
	p_media_control = NULL;

	p_tuning_space = NULL;
	p_filter_graph = NULL;
	p_system_dev_enum = NULL;
	p_network_provider = p_tuner_device = p_capture_device = NULL;
	p_sample_grabber = p_mpeg_demux = p_transport_info = NULL;
	p_scanning_tuner = NULL;
	p_grabber = NULL;

    m_fGraphFailure = FALSE;
}


// Destructor
CBDAFilterGraph::~CBDAFilterGraph()
{

}




HRESULT CBDAFilterGraph::GetName(LPWSTR pWName, LPSTR* psz_bstr_name)
{
	HRESULT     hr = S_OK;

	int i_name_len = WideCharToMultiByte(CP_ACP, 0, pWName,
		-1, *psz_bstr_name, 0, NULL, NULL);
	*psz_bstr_name = new char[i_name_len];
	i_name_len = WideCharToMultiByte(CP_ACP, 0, pWName,
		-1, *psz_bstr_name, i_name_len, NULL, NULL);


	return S_OK;
}



void CBDAFilterGraph::msg_Warn(LPCTSTR p , LPCTSTR  psz_format, ...)
{
	va_list args;
	va_start(args, psz_format);
	int const arraysize = 300;
	TCHAR pszDest[arraysize];
	size_t cbDest = arraysize * sizeof(TCHAR);

	HRESULT hr = StringCchVPrintf(pszDest, cbDest, psz_format, args);
	if (hr == S_OK) {
		OutputDebugString(pszDest);
		OutputDebugString(TEXT("\r\n"));
	}
	va_end(args);
}
#define VLC_FEC(a,b)   (((a) << 16u) | (b))
#define VLC_FEC_AUTO   0xFFFFFFFF
#define VLC_GUARD(a,b) (((a) << 16u) | (b))
#define VLC_GUARD_AUTO 0xFFFFFFFF

static BinaryConvolutionCodeRate dvb_parse_fec(LONG fec)
{
	switch (fec)
	{
	case VLC_FEC(1, 2): return BDA_BCC_RATE_1_2;
	case VLC_FEC(2, 3): return BDA_BCC_RATE_2_3;
	case VLC_FEC(3, 4): return BDA_BCC_RATE_3_4;
	case VLC_FEC(5, 6): return BDA_BCC_RATE_5_6;
	case VLC_FEC(7, 8): return BDA_BCC_RATE_7_8;
	}
	return BDA_BCC_RATE_NOT_SET;
}

static GuardInterval dvb_parse_guard(LONG guard)
{
	switch (guard)
	{
	case VLC_GUARD(1, 4): return BDA_GUARD_1_4;
	case VLC_GUARD(1, 8): return BDA_GUARD_1_8;
	case VLC_GUARD(1, 16): return BDA_GUARD_1_16;
	case VLC_GUARD(1, 32): return BDA_GUARD_1_32;
	}
	return BDA_GUARD_NOT_SET;
}

static TransmissionMode dvb_parse_transmission(int transmit)
{
	switch (transmit)
	{
	case 2: return BDA_XMIT_MODE_2K;
	case 8: return BDA_XMIT_MODE_8K;
	}
	return BDA_XMIT_MODE_NOT_SET;
}

static HierarchyAlpha dvb_parse_hierarchy(int hierarchy)
{
	switch (hierarchy)
	{
	case 1: return BDA_HALPHA_1;
	case 2: return BDA_HALPHA_2;
	case 4: return BDA_HALPHA_4;
	}
	return BDA_HALPHA_NOT_SET;
}





/*****************************************************************************
* Pop the stream of data
*****************************************************************************/
size_t CBDAFilterGraph::Pop(void *buf, size_t len)
{
	return output.Pop(buf, len);
}
LONG CBDAFilterGraph::Receive(void* buf, LONG len)
{
	return output.Pop(buf, len);
}
/******************************************************************************
* SampleCB - Callback when the Sample Grabber hasKSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT a sample
******************************************************************************/
STDMETHODIMP CBDAFilterGraph::SampleCB(double /*date*/, IMediaSample *p_sample)
{
	if (p_sample->IsDiscontinuity() == S_OK)
		msg_Warn(p_access, TEXT("BDA SampleCB: Sample Discontinuity."));

	const size_t i_sample_size = p_sample->GetActualDataLength();

	/* The buffer memory is owned by the media sample object, and is automatically
	* released when the media sample is destroyed. The caller should not free or
	* reallocate the buffer. */
	BYTE *p_sample_data;
	p_sample->GetPointer(&p_sample_data);

	if (i_sample_size > 0 && p_sample_data)
	{
		BLOCK *p_block = block_Alloc(i_sample_size);

		if (p_block)
		{
			memcpy(p_block->p_buffer, p_sample_data, i_sample_size);
			output.Push(p_block);
		}
	}
	return S_OK;
}

STDMETHODIMP CBDAFilterGraph::BufferCB(double /*date*/, BYTE* /*buffer*/,
	long /*buffer_len*/)
{
	return E_FAIL;
}






#ifdef REGISTER_FILTERGRAPH

// Adds a DirectShow filter graph to the Running Object Table,
// allowing GraphEdit to "spy" on a remote filter graph.
HRESULT CBDAFilterGraph::AddGraphToRot(
        IUnknown *pUnkGraph, 
        DWORD *pdwRegister
        ) 
{
    CComPtr <IMoniker>              pMoniker;
    CComPtr <IRunningObjectTable>   pROT;
    WCHAR wsz[128];
    HRESULT hr;

    if (FAILED(GetRunningObjectTable(0, &pROT)))
        return E_FAIL;

	StringCchPrintf(wsz,128, L"FilterGraph %08x pid %08x\0", (DWORD_PTR) pUnkGraph,
              GetCurrentProcessId());

    hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) 
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, 
                            pMoniker, pdwRegister);
        
    return hr;
}

// Removes a filter graph from the Running Object Table
void CBDAFilterGraph::RemoveGraphFromRot(
        DWORD pdwRegister
        )
{
    CComPtr <IRunningObjectTable> pROT;

    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
        pROT->Revoke(pdwRegister);

}

#endif
void CBDAFilterGraph::msg_Dbg(LPCTSTR  p,LPCTSTR  psz_format, ...)
{
	va_list args;
	va_start(args, psz_format);
	int const arraysize = 300;
	TCHAR pszDest[arraysize];
	size_t cbDest = arraysize * sizeof(TCHAR);

	HRESULT hr = StringCchVPrintf(pszDest, cbDest, psz_format, args);
	if (hr == S_OK) {
		OutputDebugString(pszDest);
		OutputDebugString(TEXT("\r\n"));
	}
	va_end(args);
}

/*****************************************************************************
* Connect is called from Build to enumerate and connect pins
*****************************************************************************/
HRESULT CBDAFilterGraph::Connect(IBaseFilter* p_upstream, IBaseFilter* p_downstream)
{
	HRESULT             hr = E_FAIL;
	class localComPtr
	{
	public:
		IPin*      p_pin_upstream;
		IPin*      p_pin_downstream;
		IEnumPins* p_pin_upstream_enum;
		IEnumPins* p_pin_downstream_enum;
		IPin*      p_pin_temp;
		char*      psz_upstream;
		char*      psz_downstream;

		localComPtr() :
			p_pin_upstream(NULL), p_pin_downstream(NULL),
			p_pin_upstream_enum(NULL), p_pin_downstream_enum(NULL),
			p_pin_temp(NULL),
			psz_upstream(NULL),
			psz_downstream(NULL)
		{ };
		~localComPtr()
		{
			if (p_pin_temp)
				p_pin_temp->Release();
			if (p_pin_downstream)
				p_pin_downstream->Release();
			if (p_pin_upstream)
				p_pin_upstream->Release();
			if (p_pin_downstream_enum)
				p_pin_downstream_enum->Release();
			if (p_pin_upstream_enum)
				p_pin_upstream_enum->Release();
			if (psz_upstream)
				delete[] psz_upstream;
			if (psz_downstream)
				delete[] psz_downstream;
		}
	} l;

	PIN_DIRECTION pin_dir;

	//msg_Dbg( p_access, TEXT("Connect: entering" );
	hr = p_upstream->EnumPins(&l.p_pin_upstream_enum);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Connect: "\
			"Cannot get upstream filter enumerator: hr=0x%8lx"), hr);
		return hr;
	}

	do
	{
		/* Release l.p_pin_upstream before next iteration */
		if (l.p_pin_upstream)
			l.p_pin_upstream->Release();
		l.p_pin_upstream = NULL;
		if (!l.p_pin_upstream_enum) break;
		hr = l.p_pin_upstream_enum->Next(1, &l.p_pin_upstream, 0);
		if (hr != S_OK) break;

		//msg_Dbg( p_access, TEXT("Connect: get pin name");
		hr = GetPinName(l.p_pin_upstream, &l.psz_upstream);
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("Connect: "\
				"Cannot GetPinName: hr=0x%8lx"), hr);
			return hr;
		}
		//msg_Dbg( p_access, TEXT("Connect: p_pin_upstream = %s", l.psz_upstream );

		hr = l.p_pin_upstream->QueryDirection(&pin_dir);
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("Connect: "\
				"Cannot get upstream filter pin direction: hr=0x%8lx"), hr);
			return hr;
		}

		hr = l.p_pin_upstream->ConnectedTo(&l.p_pin_downstream);
		if (SUCCEEDED(hr))
		{
			l.p_pin_downstream->Release();
			l.p_pin_downstream = NULL;
		}

		if (FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
		{
			msg_Warn(p_access, TEXT("Connect: "\
				"Cannot check upstream filter connection: hr=0x%8lx"), hr);
			return hr;
		}

		if ((pin_dir == PINDIR_OUTPUT) && (hr == VFW_E_NOT_CONNECTED))
		{
			/* The upstream pin is not yet connected so check each pin on the
			* downstream filter */
			//msg_Dbg( p_access, TEXT("Connect: enumerating downstream pins" );
			hr = p_downstream->EnumPins(&l.p_pin_downstream_enum);
			if (FAILED(hr))
			{
				msg_Warn(p_access, TEXT("Connect: Cannot get "\
					"downstream filter enumerator: hr=0x%8lx"), hr);
				return hr;
			}

			do
			{
				/* Release l.p_pin_downstream before next iteration */
				if (l.p_pin_downstream)
					l.p_pin_downstream->Release();
				l.p_pin_downstream = NULL;
				if (!l.p_pin_downstream_enum) break;
				hr = l.p_pin_downstream_enum->Next(1, &l.p_pin_downstream, 0);
				if (hr != S_OK) break;

				//msg_Dbg( p_access, TEXT("Connect: get pin name");
				hr = GetPinName(l.p_pin_downstream, &l.psz_downstream);
				if (FAILED(hr))
				{
					msg_Warn(p_access, TEXT("Connect: "\
						"Cannot GetPinName: hr=0x%8lx"), hr);
					return hr;
				}
				/*
				msg_Dbg( p_access, TEXT("Connect: Trying p_downstream = %s",
				l.psz_downstream );
				*/

				hr = l.p_pin_downstream->QueryDirection(&pin_dir);
				if (FAILED(hr))
				{
					msg_Warn(p_access, TEXT("Connect: Cannot get "\
						"downstream filter pin direction: hr=0x%8lx"), hr);
					return hr;
				}

				/* Looking for a free Pin to connect to
				* A connected Pin may have an reference count > 1
				* so Release and nullify the pointer */
				hr = l.p_pin_downstream->ConnectedTo(&l.p_pin_temp);
				if (SUCCEEDED(hr))
				{
					l.p_pin_temp->Release();
					l.p_pin_temp = NULL;
				}

				if (hr != VFW_E_NOT_CONNECTED)
				{
					if (FAILED(hr))
					{
						msg_Warn(p_access, TEXT("Connect: Cannot check "\
							"downstream filter connection: hr=0x%8lx"), hr);
						return hr;
					}
				}

				if ((pin_dir == PINDIR_INPUT) &&
					(hr == VFW_E_NOT_CONNECTED))
				{
					//msg_Dbg( p_access, TEXT("Connect: trying to connect pins" );

					hr = p_filter_graph->ConnectDirect(l.p_pin_upstream,
						l.p_pin_downstream, NULL);
					if (SUCCEEDED(hr))
					{
						/* If we arrive here then we have a matching pair of
						* pins. */
						return S_OK;
					}
				}
				/* If we arrive here it means this downstream pin is not
				* suitable so try the next downstream pin.
				* l.p_pin_downstream is released at the top of the loop */
			} while (true);
			/* If we arrive here then we ran out of pins before we found a
			* suitable one. Release outstanding refcounts */
			if (l.p_pin_downstream_enum)
				l.p_pin_downstream_enum->Release();
			l.p_pin_downstream_enum = NULL;
			if (l.p_pin_downstream)
				l.p_pin_downstream->Release();
			l.p_pin_downstream = NULL;
		}
		/* If we arrive here it means this upstream pin is not suitable
		* so try the next upstream pin
		* l.p_pin_upstream is released at the top of the loop */
	} while (true);
	/* If we arrive here it means we did not find any pair of suitable pins
	* Outstanding refcounts are released in the destructor */
	//msg_Dbg( p_access, TEXT("Connect: No pins connected" );
	return E_FAIL;
}
/******************************************************************************
* FindFilter
* Looks up all filters in a category and connects to the upstream filter until
* a successful match is found. The index of the connected filter is returned.
* On subsequent calls, this can be used to start from that point to find
* another match.
* This is used when the graph does not run because a tuner device is in use so
* another one needs to be selected.
******************************************************************************/
HRESULT CBDAFilterGraph::FindFilter(REFCLSID this_clsid, long* i_moniker_used,
	IBaseFilter* p_upstream, IBaseFilter** p_p_downstream)
{
	HRESULT                 hr = S_OK;
	int                     i_moniker_index = -1;
	class localComPtr
	{
	public:
		IEnumMoniker*  p_moniker_enum;
		IMoniker*      p_moniker;
		IBindCtx*      p_bind_context;
		IPropertyBag*  p_property_bag;
		char*          psz_upstream;
		int            i_upstream_len;

		char*          psz_downstream;
		VARIANT        var_bstr;
		int            i_bstr_len;
		localComPtr() :
			p_moniker_enum(NULL),
			p_moniker(NULL),
			p_bind_context(NULL),
			p_property_bag(NULL),
			psz_upstream(NULL),
			psz_downstream(NULL)
		{
			::VariantInit(&var_bstr);
		}
		~localComPtr()
		{
			if (p_moniker_enum)
				p_moniker_enum->Release();
			if (p_moniker)
				p_moniker->Release();
			if (p_bind_context)
				p_bind_context->Release();
			if (p_property_bag)
				p_property_bag->Release();
			if (psz_upstream)
				delete[] psz_upstream;
			if (psz_downstream)
				delete[] psz_downstream;

			::VariantClear(&var_bstr);
		}
	} l;

	/* create system_dev_enum the first time through, or preserve the
	* existing one to loop through classes */
	if (!p_system_dev_enum)
	{
		msg_Dbg(p_access, TEXT("FindFilter: Create p_system_dev_enum"));
		hr = ::CoCreateInstance(CLSID_SystemDeviceEnum, 0, CLSCTX_INPROC,
			IID_ICreateDevEnum, reinterpret_cast<void**>(&p_system_dev_enum));
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("FindFilter: "\
				"Cannot CoCreate SystemDeviceEnum: hr=0x%8lx"), hr);
			return hr;
		}
	}

	msg_Dbg(p_access, TEXT("FindFilter: Create p_moniker_enum"));
	hr = p_system_dev_enum->CreateClassEnumerator(this_clsid,
		&l.p_moniker_enum, 0);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("FindFilter: "\
			"Cannot CreateClassEnumerator: hr=0x%8lx"), hr);
		return hr;
	}

	msg_Dbg(p_access, TEXT("FindFilter: get filter name"));
	hr = GetFilterName(p_upstream, &l.psz_upstream);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("FindFilter: "\
			"Cannot GetFilterName: hr=0x%8lx"), hr);
		return hr;
	}

	msg_Dbg(p_access, TEXT("FindFilter: "\
		"called with i_moniker_used=%ld, " \
		"p_upstream = %s"), *i_moniker_used, l.psz_upstream);

	do
	{
		/* We are overwriting l.p_moniker so we should Release and nullify
		* It is important that p_moniker and p_property_bag are fully released */
		msg_Dbg(p_access, TEXT("FindFilter: top of main loop"));
		if (l.p_property_bag)
			l.p_property_bag->Release();
		l.p_property_bag = NULL;
		msg_Dbg(p_access, TEXT("FindFilter: releasing bind context"));
		if (l.p_bind_context)
			l.p_bind_context->Release();
		l.p_bind_context = NULL;
		msg_Dbg(p_access, TEXT("FindFilter: releasing moniker"));
		if (l.p_moniker)
			l.p_moniker->Release();
		msg_Dbg(p_access, TEXT("FindFilter: null moniker"));
		l.p_moniker = NULL;

		msg_Dbg(p_access, TEXT("FindFilter: quit if no enum"));
		if (!l.p_moniker_enum) break;
		msg_Dbg(p_access, TEXT("FindFilter: trying a moniker"));
		hr = l.p_moniker_enum->Next(1, &l.p_moniker, 0);
		if (hr != S_OK) break;

		i_moniker_index++;

		/* Skip over devices already found on previous calls */
		msg_Dbg(p_access, TEXT("FindFilter: skip previously found devices"));

		if (i_moniker_index <= *i_moniker_used) continue;
		*i_moniker_used = i_moniker_index;

		/* l.p_bind_context is Released at the top of the loop */
		msg_Dbg(p_access, TEXT("FindFilter: create bind context"));
		hr = CreateBindCtx(0, &l.p_bind_context);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: "\
				"Cannot create bind_context, trying another: hr=0x%8lx"), hr);
			continue;
		}

		msg_Dbg(p_access, TEXT("FindFilter: try to create downstream filter"));
		*p_p_downstream = NULL;
		hr = l.p_moniker->BindToObject(l.p_bind_context, NULL, IID_IBaseFilter,
			reinterpret_cast<void**>(p_p_downstream));
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: "\
				"Cannot bind to downstream, trying another: hr=0x%8lx"), hr);
			continue;
		}

#ifdef DEBUG_MONIKER_NAME
		msg_Dbg(p_access, TEXT("FindFilter: get downstream filter name");

		WCHAR*  pwsz_downstream = NULL;

		hr = l.p_moniker->GetDisplayName(l.p_bind_context, NULL,
			&pwsz_downstream);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: "\
				"Cannot get display name, trying another: hr=0x%8lx", hr);
			continue;
		}

		if (l.psz_downstream)
			delete[] l.psz_downstream;
		l.i_bstr_len = WideCharToMultiByte(CP_ACP, 0, pwsz_downstream, -1,
			l.psz_downstream, 0, NULL, NULL);
		l.psz_downstream = new char[l.i_bstr_len];
		l.i_bstr_len = WideCharToMultiByte(CP_ACP, 0, pwsz_downstream, -1,
			l.psz_downstream, l.i_bstr_len, NULL, NULL);

		LPMALLOC p_alloc;
		::CoGetMalloc(1, &p_alloc);
		p_alloc->Free(pwsz_downstream);
		p_alloc->Release();
#else
		l.psz_downstream = _strdup("Downstream");
#endif

		/* l.p_property_bag is released at the top of the loop */
		msg_Dbg(p_access, TEXT("FindFilter: "\
			"Moniker name is  %s, binding to storage"), l.psz_downstream);
		hr = l.p_moniker->BindToStorage(l.p_bind_context, NULL,
			IID_IPropertyBag, reinterpret_cast<void**>(&l.p_property_bag));
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: "\
				"Cannot Bind to Property Bag: hr=0x%8lx"), hr);
			continue;
		}

		msg_Dbg(p_access, TEXT("FindFilter: read friendly name"));
		hr = l.p_property_bag->Read(L"FriendlyName", &l.var_bstr, NULL);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: "\
				"Cannot read friendly name, next?: hr=0x%8lx"), hr);
			continue;
		}

		msg_Dbg(p_access, TEXT("FindFilter: add filter to graph"));
		hr = p_filter_graph->AddFilter(*p_p_downstream, l.var_bstr.bstrVal);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: "\
				"Cannot add filter, trying another: hr=0x%8lx"), hr);
			continue;
		}

		msg_Dbg(p_access, TEXT("FindFilter: "\
			"Trying to Connect %s to %s"), l.psz_upstream, l.psz_downstream);
		hr = Connect(p_upstream, *p_p_downstream);
		if (SUCCEEDED(hr))
		{
			msg_Dbg(p_access, TEXT("FindFilter: Connected %s"), l.psz_downstream);
			return S_OK;
		}

		/* Not the filter we want so unload and try the next one */
		/* Warning: RemoveFilter does an undocumented Release()
		* on pointer but does not set it to NULL */
		msg_Dbg(p_access, TEXT("FindFilter: Removing filter"));
		hr = p_filter_graph->RemoveFilter(*p_p_downstream);
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("FindFilter: "\
				"Failed unloading Filter: hr=0x%8lx"), hr);
			continue;
		}
		msg_Dbg(p_access, TEXT("FindFilter: trying another"));
	} while (true);

	/* nothing found */
	msg_Warn(p_access, TEXT("FindFilter: No filter connected"));
	hr = E_FAIL;
	return hr;
}
LPTSTR  CBDAFilterGraph::var_GetNonEmptyString(LPCTSTR  p, LPCTSTR  pszkey)
{
	LPTSTR pc = new TCHAR[1];
	*pc = '\0';
	return pc;
}
/*****************************************************************************
* GetSystem
* helper function
*****************************************************************************/
unsigned CBDAFilterGraph::GetSystem(REFCLSID clsid)
{
	unsigned sys = 0;

	if (clsid == CLSID_DVBTNetworkProvider)
		sys = DVB_T;
	if (clsid == CLSID_DVBCNetworkProvider)
		sys = DVB_C;
	if (clsid == CLSID_DVBSNetworkProvider)
		sys = DVB_S;
	if (clsid == CLSID_ATSCNetworkProvider)
		sys = ATSC;
//	if (clsid == CLSID_DigitalCableNetworkType)
//		sys = CQAM;

	return sys;
}
/******************************************************************************
* removes each filter from the graph
******************************************************************************/
HRESULT CBDAFilterGraph::Destroy()
{
	HRESULT hr = S_OK;
	ULONG mem_ref = 0;
	//    msg_Dbg( p_access, TEXT("Destroy: media control 1" );
	if (p_media_control)
		p_media_control->StopWhenReady(); /* Instead of Stop() */

										  //    msg_Dbg( p_access, TEXT("Destroy: deregistering graph" );
	if (d_graph_register)
		Deregister();

	//    msg_Dbg( p_access, TEXT("Destroy: calling Empty" );
	output.Empty();

	//    msg_Dbg( p_access, TEXT("Destroy: TIF" );
	if (p_transport_info)
	{
		/* Warning: RemoveFilter does an undocumented Release()
		* on pointer but does not set it to NULL */
		hr = p_filter_graph->RemoveFilter(p_transport_info);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Failed unloading TIF: hr=0x%8lx"), hr);
		}
		p_transport_info = NULL;
	}

	//    msg_Dbg( p_access, TEXT("Destroy: demux" );
	if (p_mpeg_demux)
	{
		p_filter_graph->RemoveFilter(p_mpeg_demux);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Failed unloading demux: hr=0x%8lx"), hr);
		}
		p_mpeg_demux = NULL;
	}

	//    msg_Dbg( p_access, TEXT("Destroy: sample grabber" );
	if (p_grabber)
	{
		mem_ref = p_grabber->Release();
		if (mem_ref != 0)
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Sample grabber mem_ref (varies): mem_ref=%ld"), mem_ref);
		}
		p_grabber = NULL;
	}

	//    msg_Dbg( p_access, TEXT("Destroy: sample grabber filter" );
	if (p_sample_grabber)
	{
		hr = p_filter_graph->RemoveFilter(p_sample_grabber);
		p_sample_grabber = NULL;
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Failed unloading sampler: hr=0x%8lx"), hr);
		}
	}

	//    msg_Dbg( p_access, TEXT("Destroy: capture device" );
	if (p_capture_device)
	{
		p_filter_graph->RemoveFilter(p_capture_device);
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Failed unloading capture device: hr=0x%8lx"), hr);
		}
		p_capture_device = NULL;
	}

	//    msg_Dbg( p_access, TEXT("Destroy: tuner device" );
	if (p_tuner_device)
	{
		//msg_Dbg( p_access, TEXT("Destroy: remove filter on tuner device" );
		hr = p_filter_graph->RemoveFilter(p_tuner_device);
		//msg_Dbg( p_access, TEXT("Destroy: force tuner device to NULL" );
		p_tuner_device = NULL;
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Failed unloading tuner device: hr=0x%8lx"), hr);
		}
	}

	//    msg_Dbg( p_access, TEXT("Destroy: scanning tuner" );
	if (p_scanning_tuner)
	{
		mem_ref = p_scanning_tuner->Release();
		if (mem_ref != 0)
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Scanning tuner mem_ref (normally 2 if warm, "\
				"3 if active): mem_ref=%ld"), mem_ref);
		}
		p_scanning_tuner = NULL;
	}

	//    msg_Dbg( p_access, TEXT("Destroy: net provider" );
	if (p_network_provider)
	{
		hr = p_filter_graph->RemoveFilter(p_network_provider);
		p_network_provider = NULL;
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Failed unloading net provider: hr=0x%8lx"), hr);
		}
	}

	//    msg_Dbg( p_access, TEXT("Destroy: filter graph" );
	if (p_filter_graph)
	{
		mem_ref = p_filter_graph->Release();
		if (mem_ref != 0)
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Filter graph mem_ref (normally 1 if active): mem_ref=%ld"),
				mem_ref);
		}
		p_filter_graph = NULL;
	}

	/* first call to FindFilter creates p_system_dev_enum */

	//    msg_Dbg( p_access, TEXT("Destroy: system dev enum" );
	if (p_system_dev_enum)
	{
		mem_ref = p_system_dev_enum->Release();
		if (mem_ref != 0)
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"System_dev_enum mem_ref: mem_ref=%ld"), mem_ref);
		}
		p_system_dev_enum = NULL;
	}

	//    msg_Dbg( p_access, TEXT("Destroy: media control 2" );
	if (p_media_control)
	{
		msg_Dbg(p_access, TEXT("Destroy: release media control"));
		mem_ref = p_media_control->Release();
		if (mem_ref != 0)
		{
			msg_Dbg(p_access, TEXT("Destroy: "\
				"Media control mem_ref: mem_ref=%ld"), mem_ref);
		}
		msg_Dbg(p_access, TEXT("Destroy: force media control to NULL"));
		p_media_control = NULL;
	}

	d_graph_register = 0;
	l_tuner_used = -1;
	guid_network_type = GUID_NULL;

	//    msg_Dbg( p_access, TEXT("Destroy: returning" );
	return S_OK;
}
/*****************************************************************************
* Set DVB-T
*
* This provides the tune request that everything else is built upon.
*
* Stores the tune request to the scanning tuner, where it is pulled out by
* dvb_tune a/k/a SubmitTuneRequest.
******************************************************************************/
int CBDAFilterGraph::SetDVBT(long l_frequency, unsigned __int32  fec_hp, unsigned __int32 fec_lp,
	long l_bandwidth, int transmission, unsigned __int32 guard, int hierarchy)
{
	HRESULT hr = S_OK;

	class localComPtr
	{
	public:
		ITuneRequest*       p_tune_request;
		IDVBTuneRequest*    p_dvb_tune_request;
		IDVBTLocator*       p_dvbt_locator;
		IDVBTuningSpace2*   p_dvb_tuning_space;
		localComPtr() :
			p_tune_request(NULL),
			p_dvb_tune_request(NULL),
			p_dvbt_locator(NULL),
			p_dvb_tuning_space(NULL)
		{};
		~localComPtr()
		{
			if (p_tune_request)
				p_tune_request->Release();
			if (p_dvb_tune_request)
				p_dvb_tune_request->Release();
			if (p_dvbt_locator)
				p_dvbt_locator->Release();
			if (p_dvb_tuning_space)
				p_dvb_tuning_space->Release();
		}
	} l;

	/* create local dvbt-specific tune request and locator
	* then put it to existing scanning tuner */
	BinaryConvolutionCodeRate i_hp_fec = dvb_parse_fec(fec_hp);
	BinaryConvolutionCodeRate i_lp_fec = dvb_parse_fec(fec_lp);
	GuardInterval i_guard = dvb_parse_guard(guard);
	TransmissionMode i_transmission = dvb_parse_transmission(transmission);
	HierarchyAlpha i_hierarchy = dvb_parse_hierarchy(hierarchy);

	/* try to set p_scanning_tuner */
	msg_Dbg(p_access, TEXT("SetDVBT: set up scanning tuner"));
	hr = Check(CLSID_DVBTNetworkProvider);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot create Tuning Space: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	if (!p_scanning_tuner)
	{
		msg_Warn(p_access, TEXT("SetDVBT: Cannot get scanning tuner"));
		return VLC_EGENERIC;
	}

	hr = p_scanning_tuner->get_TuneRequest(&l.p_tune_request);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot get Tune Request: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, TEXT("SetDVBT: Creating DVB tune request"));

	// WARNING flecoqui
	hr = l.p_tune_request->QueryInterface(IID_IDVBTuneRequest,
	//	hr = l.p_tune_request->QueryInterface(__uuidof(IDVBTuneRequest),
			reinterpret_cast<void**>(&l.p_dvb_tune_request));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot QI for IDVBTuneRequest: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	l.p_dvb_tune_request->put_ONID(-1);
	l.p_dvb_tune_request->put_SID(1025);
	l.p_dvb_tune_request->put_TSID(-1);

	msg_Dbg(p_access, TEXT("SetDVBT: get TS"));
	hr = p_scanning_tuner->get_TuningSpace(&p_tuning_space);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetDVBT: "\
			"cannot get tuning space: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, TEXT("SetDVBT: QI to DVBT TS"));
	// WARNING flecoqui
	hr = p_tuning_space->QueryInterface(IID_IDVBTuningSpace2,
	//	hr = p_tuning_space->QueryInterface(__uuidof(IDVBTuningSpace2),
			reinterpret_cast<void**>(&l.p_dvb_tuning_space));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot QI for IDVBTuningSpace2: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, TEXT("SetDVBT: Creating local locator"));
	// WARNING flecoqui
	hr = ::CoCreateInstance(CLSID_DVBTLocator, 0, CLSCTX_INPROC,
	//	hr = ::CoCreateInstance(__uuidof(IDVBTLocator), 0, CLSCTX_INPROC,
			IID_IDVBTLocator, reinterpret_cast<void**>(&l.p_dvbt_locator));
//		__uuidof(IDVBTLocator), reinterpret_cast<void**>(&l.p_dvbt_locator));

	//	CComPtr <IDVBTLocator> pDVBTLocator;
//		hr = l.p_dvbt_locator.CoCreateInstance(_uuidof(IDVBTLocator));
		//hr = pDVBTLocator.CoCreateInstance(_uuidof(IDVBTLocator));
		//	hr = pATSCLocator.CoCreateInstance(CLSID_ATSCLocator);
	//	hr = pATSCLocator.CoCreateInstance(_uuidof(ATSCLocator));
	//	l.p_dvbt_locator = pDVBTLocator;
		if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot create the DVBT Locator: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	hr = l.p_dvb_tuning_space->put_SystemType(DVB_Terrestrial);
	if (SUCCEEDED(hr) && l_frequency > 0)
		hr = l.p_dvbt_locator->put_CarrierFrequency(l_frequency);
	if (SUCCEEDED(hr) && l_bandwidth > 0)
		hr = l.p_dvbt_locator->put_Bandwidth(l_bandwidth);
	if (SUCCEEDED(hr) && i_hp_fec != BDA_BCC_RATE_NOT_SET)
		hr = l.p_dvbt_locator->put_InnerFECRate(i_hp_fec);
	if (SUCCEEDED(hr) && i_lp_fec != BDA_BCC_RATE_NOT_SET)
		hr = l.p_dvbt_locator->put_LPInnerFECRate(i_lp_fec);
	if (SUCCEEDED(hr) && i_guard != BDA_GUARD_NOT_SET)
		hr = l.p_dvbt_locator->put_Guard(i_guard);
	if (SUCCEEDED(hr) && i_transmission != BDA_XMIT_MODE_NOT_SET)
		hr = l.p_dvbt_locator->put_Mode(i_transmission);
	if (SUCCEEDED(hr) && i_hierarchy != BDA_HALPHA_NOT_SET)
		hr = l.p_dvbt_locator->put_HAlpha(i_hierarchy);

	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot set tuning parameters on Locator: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, TEXT("SetDVBT: putting DVBT locator into local tune request"));

	hr = l.p_dvb_tune_request->put_Locator(l.p_dvbt_locator);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot put the locator: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, TEXT("SetDVBT: putting local Tune Request to scanning tuner"));
	hr = p_scanning_tuner->Validate(l.p_dvb_tune_request);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetDVBT: "\
			"Tune Request cannot be validated: hr=0x%8lx"), hr);
	}
	/* increments ref count for scanning tuner */
	hr = p_scanning_tuner->put_TuneRequest(l.p_dvb_tune_request);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetDVBT: "\
			"Cannot put the tune request: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, TEXT("SetDVBT: return success"));
	return VLC_SUCCESS;
}
/******************************************************************************
* Build
*******************************************************************************
* Build the Filter Graph
*
* connects filters and
* creates the media control and registers the graph
* on success, sets globals:
* d_graph_register, p_media_control, p_grabber, p_sample_grabber,
* p_mpeg_demux, p_transport_info
******************************************************************************/
HRESULT CBDAFilterGraph::Build()
{
	HRESULT hr = S_OK;
	long            l_capture_used;
	long            l_tif_used;
	AM_MEDIA_TYPE   grabber_media_type;

	class localComPtr
	{
	public:
		ITuningSpaceContainer*  p_tuning_space_container;
		localComPtr() :
			p_tuning_space_container(NULL)
		{};
		~localComPtr()
		{
			if (p_tuning_space_container)
				p_tuning_space_container->Release();
		}
	} l;

	msg_Dbg(p_access, TEXT("Build: entering"));

	/* at this point, you've connected to a scanning tuner of the right
	* network type.
	*/
	if (!p_scanning_tuner || !p_tuner_device)
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Scanning Tuner does not exist"));
		return E_FAIL;
	}

	hr = p_scanning_tuner->get_TuneRequest(&p_tune_request);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: no tune request"));
		return hr;
	}
	hr = p_scanning_tuner->get_TuningSpace(&p_tuning_space);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: no tuning space"));
		return hr;
	}
	hr = p_tuning_space->get__NetworkType(&guid_network_type);


	/* Always look for all capture devices to match the Network Tuner */
	l_capture_used = -1;
	msg_Dbg(p_access, TEXT("Build: "\
		"Calling FindFilter for KSCATEGORY_BDA_RECEIVER_COMPONENT with "\
		"p_tuner_device; l_capture_used=%ld"), l_capture_used);
	hr = FindFilter(KSCATEGORY_BDA_RECEIVER_COMPONENT, &l_capture_used,
		p_tuner_device, &p_capture_device);
	if (FAILED(hr))
	{
		/* Some BDA drivers do not provide a Capture Device Filter so force
		* the Sample Grabber to connect directly to the Tuner Device */
		msg_Dbg(p_access, TEXT("Build: "\
			"Cannot find Capture device. Connect to tuner "\
			"and AddRef() : hr=0x%8lx"), hr);
		p_capture_device = p_tuner_device;
		p_capture_device->AddRef();
	}

	if (p_sample_grabber)
		p_sample_grabber->Release();
	p_sample_grabber = NULL;
	/* Insert the Sample Grabber to tap into the Transport Stream. */
	hr = ::CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, reinterpret_cast<void**>(&p_sample_grabber));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot load Sample Grabber Filter: hr=0x%8lx"), hr);
		return hr;
	}

	hr = p_filter_graph->AddFilter(p_sample_grabber, L"Sample Grabber");
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot add Sample Grabber Filter to graph: hr=0x%8lx"), hr);
		return hr;
	}

	/* create the sample grabber */
	if (p_grabber)
		p_grabber->Release();
	p_grabber = NULL;
	hr = p_sample_grabber->QueryInterface(IID_ISampleGrabber,
		reinterpret_cast<void**>(&p_grabber));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot QI Sample Grabber Filter: hr=0x%8lx"), hr);
		return hr;
	}

	/* Try the possible stream type */
	hr = E_FAIL;
	for (int i = 0; i < 2; i++)
	{
		ZeroMemory(&grabber_media_type, sizeof(AM_MEDIA_TYPE));
		grabber_media_type.majortype = MEDIATYPE_Stream;
		grabber_media_type.subtype = i == 0 ? MEDIASUBTYPE_MPEG2_TRANSPORT : KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT;
		msg_Dbg(p_access, TEXT("Build: "
			"Trying connecting with subtype %s"),
			i == 0 ? TEXT("MEDIASUBTYPE_MPEG2_TRANSPORT") : TEXT("KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT"));
		hr = p_grabber->SetMediaType(&grabber_media_type);
		if (SUCCEEDED(hr))
		{
			hr = Connect(p_capture_device, p_sample_grabber);
			if (SUCCEEDED(hr))
			{
				msg_Dbg(p_access, TEXT("Build: "\
					"Connected capture device to sample grabber"));
				break;
			}
			msg_Warn(p_access, TEXT("Build: "\
				// FLECOQUI
				//"Cannot connect Sample Grabber to Capture device: hr=0x%8lx (try %d/2)", hr, 1 + i);
			"Cannot connect Sample Grabber to Capture device: "));
		}
		else
		{
			msg_Warn(p_access, TEXT("Build: "\
				"Cannot set media type on grabber filter: hr=0x%8lx (try %d/2"), hr, 1 + i);
		}
	}
	msg_Dbg(p_access, TEXT("Build: This is where it used to return upon success"));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot use capture device: hr=0x%8lx"), hr);
		return hr;
	}

	/* We need the MPEG2 Demultiplexer even though we are going to use the VLC
	* TS demuxer. The TIF filter connects to the MPEG2 demux and works with
	* the Network Provider filter to set up the stream */
	//msg_Dbg( p_access, TEXT("Build: using MPEG2 demux" );
	if (p_mpeg_demux)
		p_mpeg_demux->Release();
	p_mpeg_demux = NULL;
	hr = ::CoCreateInstance(CLSID_MPEG2Demultiplexer, NULL,
		CLSCTX_INPROC_SERVER, IID_IBaseFilter,
		reinterpret_cast<void**>(&p_mpeg_demux));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot CoCreateInstance MPEG2 Demultiplexer: hr=0x%8lx"), hr);
		return hr;
	}

	//msg_Dbg( p_access, TEXT("Build: adding demux" );
	hr = p_filter_graph->AddFilter(p_mpeg_demux, L"Demux");
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot add demux filter to graph: hr=0x%8lx"), hr);
		return hr;
	}

	hr = Connect(p_sample_grabber, p_mpeg_demux);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot connect demux to grabber: hr=0x%8lx"), hr);
		return hr;
	}

	//msg_Dbg( p_access, TEXT("Build: Connected sample grabber to demux" );
	/* Always look for the Transport Information Filter from the start
	* of the collection*/
	l_tif_used = -1;
	msg_Dbg(p_access, TEXT("Check: "\
		"Calling FindFilter for KSCATEGORY_BDA_TRANSPORT_INFORMATION with "\
		"p_mpeg_demux; l_tif_used=%ld"), l_tif_used);


	hr = FindFilter(KSCATEGORY_BDA_TRANSPORT_INFORMATION, &l_tif_used,
		p_mpeg_demux, &p_transport_info);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot load TIF onto demux: hr=0x%8lx"), hr);
		return hr;
	}

	/* Configure the Sample Grabber to buffer the samples continuously */
	hr = p_grabber->SetBufferSamples(true);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot set Sample Grabber to buffering: hr=0x%8lx"), hr);
		return hr;
	}

	hr = p_grabber->SetOneShot(false);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot set Sample Grabber to multi shot: hr=0x%8lx"), hr);
		return hr;
	}

	/* Second parameter to SetCallback specifies the callback method; 0 uses
	* the ISampleGrabberCB::SampleCB method, which receives an IMediaSample
	* pointer. */
	//msg_Dbg( p_access, TEXT("Build: Adding grabber callback" );
	hr = p_grabber->SetCallback(this, 0);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot set SampleGrabber Callback: hr=0x%8lx"), hr);
		return hr;
	}

	hr = Register(); /* creates d_graph_register */
	if (FAILED(hr))
	{
		d_graph_register = 0;
		msg_Dbg(p_access, TEXT("Build: "\
			"Cannot register graph: hr=0x%8lx"), hr);
	}

	/* The Media Control is used to Run and Stop the Graph */
	if (p_media_control)
		p_media_control->Release();
	p_media_control = NULL;
	hr = p_filter_graph->QueryInterface(IID_IMediaControl,
		reinterpret_cast<void**>(&p_media_control));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Build: "\
			"Cannot QI Media Control: hr=0x%8lx"), hr);
		return hr;
	}

	/* success! */
	//msg_Dbg( p_access, TEXT("Build: succeeded: hr=0x%8lx", hr );
	return S_OK;
}
int CBDAFilterGraph::SubmitTuneRequest(void)
{
	HRESULT hr;
	int i = 0;

	/* Build and Start the Graph. If a Tuner device is in use the graph will
	* fail to start. Repeated calls to Build will check successive tuner
	* devices */
	do
	{
		msg_Dbg(p_access, TEXT("SubmitTuneRequest: Building the Graph"));

		hr = Build();
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("SubmitTuneRequest: "
				"Cannot Build the Graph: hr=0x%8lx"), hr);
			return VLC_EGENERIC;
		}
		msg_Dbg(p_access, TEXT("SubmitTuneRequest: Starting the Graph"));

		hr = Start();
		if (FAILED(hr))
		{
			msg_Dbg(p_access, TEXT("SubmitTuneRequest: "
				"Cannot Start the Graph, retrying: hr=0x%8lx"), hr);
			++i;
		}
	} while (hr != S_OK && i < 10); /* give up after 10 tries */

	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SubmitTuneRequest: "
			"Failed to Start the Graph: hr=0x%8lx"), hr);
		return VLC_EGENERIC;
	}

	return VLC_SUCCESS;
}

/*****************************************************************************
* Start uses MediaControl to start the graph
*****************************************************************************/
HRESULT CBDAFilterGraph::Start()
{
	HRESULT hr = S_OK;
	OAFilterState i_state; /* State_Stopped, State_Paused, State_Running */

	msg_Dbg(p_access, TEXT("Start: entering"));

	if (!p_media_control)
	{
		msg_Warn(p_access, TEXT("Start: Media Control has not been created"));
		return E_FAIL;
	}

	msg_Dbg(p_access, TEXT("Start: Run()"));
	hr = p_media_control->Run();
	if (SUCCEEDED(hr))
	{
		msg_Dbg(p_access, TEXT("Start: Graph started, hr=0x%lx"), hr);
		return S_OK;
	}

	msg_Dbg(p_access, TEXT("Start: would not start, will retry"));
	/* Query the state of the graph - timeout after 100 milliseconds */
	while ((hr = p_media_control->GetState(100, &i_state)) != S_OK)
	{
		if (FAILED(hr))
		{
			msg_Warn(p_access,TEXT("Start: Cannot get Graph state: hr=0x%8lx"), hr);
			return hr;
		}
	}

	msg_Dbg(p_access, TEXT("Start: got state"));
	if (i_state == State_Running)
	{
		msg_Dbg(p_access, TEXT("Graph started after a delay, hr=0x%lx"), hr);
		return S_OK;
	}

	/* The Graph is not running so stop it and return an error */
	msg_Warn(p_access, TEXT("Start: Graph not started: %d"), (int)i_state);
	hr = p_media_control->StopWhenReady(); /* Instead of Stop() */
	if (FAILED(hr))
	{
		msg_Warn(p_access,
			TEXT("Start: Cannot stop Graph after Run failed: hr=0x%8lx"), hr);
		return hr;
	}

	return E_FAIL;
}
/******************************************************************************
* Check
*******************************************************************************
* Check if tuner supports this network type
*
* on success, sets globals:
* systems, l_tuner_used, p_network_provider, p_scanning_tuner, p_tuner_device,
* p_tuning_space, p_filter_graph
******************************************************************************/
HRESULT CBDAFilterGraph::Check(REFCLSID guid_this_network_type)
{
	HRESULT hr = S_OK;

	class localComPtr
	{
	public:
		ITuningSpaceContainer*  p_tuning_space_container;

		localComPtr() :
			p_tuning_space_container(NULL)
		{};
		~localComPtr()
		{
			if (p_tuning_space_container)
				p_tuning_space_container->Release();
		}
	} l;

	msg_Dbg(p_access, TEXT("Check: entering "));

	/* Note that the systems global is persistent across Destroy().
	* So we need to see if a tuner has been physically removed from
	* the system since the last Check. Before we do anything,
	* assume that this Check will fail and remove this network type
	* from systems. It will be restored if the Check passes.
	*/

	systems &= ~(GetSystem(guid_this_network_type));


	/* If we have already have a filter graph, rebuild it*/
	msg_Dbg(p_access, TEXT("Check: Destroying filter graph"));
	if (p_filter_graph)
		Destroy();
	p_filter_graph = NULL;
	// create the filter graph
	hr = ::CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, reinterpret_cast<void**>(&p_filter_graph));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Check: "\
			"Cannot CoCreate IFilterGraph: hr=0x%8lx"), hr);
		return hr;
	}
	/* First filter in the graph is the Network Provider and
	* its Scanning Tuner which takes the Tune Request */
	if (p_network_provider)
		p_network_provider->Release();
	p_network_provider = NULL;
	hr = ::CoCreateInstance(guid_this_network_type, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, reinterpret_cast<void**>(&p_network_provider));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Check: "\
			"Cannot CoCreate Network Provider: hr=0x%8lx"), hr);
		return hr;
	}

	msg_Dbg(p_access, TEXT("Check: adding Network Provider to graph"));
	hr = p_filter_graph->AddFilter(p_network_provider, L"Network Provider");
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Check: "\
			"Cannot load network provider: hr=0x%8lx"), hr);
		return hr;
	}

	/* Add the Network Tuner to the Network Provider. On subsequent calls,
	* l_tuner_used will cause a different tuner to be selected.
	*
	* To select a specific device first get the parameter that nominates the
	* device (dvb-adapter) and use the value to initialise l_tuner_used.
	* Note that dvb-adapter is 1-based, while l_tuner_used is 0-based.
	* When FindFilter returns, check the contents of l_tuner_used.
	* If it is not what was selected, then the requested device was not
	* available, so return with an error. */

	long l_adapter = -1;
	l_adapter = var_GetInteger(p_access, TEXT("dvb-adapter"));
	if (l_tuner_used < 0 && l_adapter >= 0)
		l_tuner_used = l_adapter - 1;

	/* If tuner is in cold state, we have to do a successful put_TuneRequest
	* before it will Connect. */
	msg_Dbg(p_access, TEXT("Check: Creating Scanning Tuner"));
	if (p_scanning_tuner)
		p_scanning_tuner->Release();
	p_scanning_tuner = NULL;
	//flecoqui WARNING WARNING WARNING WARNING WARNING
	hr = p_network_provider->QueryInterface(IID_IScanningTuner,
		//hr = p_network_provider->QueryInterface(__uuidof( IScanningTuner),
		reinterpret_cast<void**>(&p_scanning_tuner));
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Check: "\
			"Cannot QI Network Provider for Scanning Tuner: hr=0x%8lx"), hr);
		return hr;
	}

	/* try to set up p_scanning_tuner */
	msg_Dbg(p_access, TEXT("Check: Calling SetUpTuner"));
	hr = SetUpTuner(guid_this_network_type);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("Check: "\
			"Cannot set up scanner in Check mode: hr=0x%8lx"), hr);
		return hr;
	}

	msg_Dbg(p_access, TEXT("Check: "\
		"Calling FindFilter for KSCATEGORY_BDA_NETWORK_TUNER with "\
		"p_network_provider; l_tuner_used=%ld"), l_tuner_used);
	hr = FindFilter(KSCATEGORY_BDA_NETWORK_TUNER, &l_tuner_used,
		p_network_provider, &p_tuner_device);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Check: "\
			"Cannot load tuner device and connect network provider: "\
			"hr=0x%8lx"), hr);
		return hr;
	}

	if (l_adapter > 0 && l_tuner_used != l_adapter)
	{
		msg_Warn(p_access, TEXT("Check: "\
			"Tuner device %ld is not available"), l_adapter);
		return E_FAIL;
	}

	msg_Dbg(p_access, TEXT("Check: Using adapter %ld"), l_tuner_used);
	/* success!
	* already set l_tuner_used,
	* p_tuning_space
	*/
	msg_Dbg(p_access, TEXT("Check: check succeeded: hr=0x%8lx"), hr);
	systems |= GetSystem(guid_this_network_type);
	msg_Dbg(p_access, TEXT("Check: returning from Check mode"));
	return S_OK;
}
LONG CBDAFilterGraph::var_GetInteger(LPCTSTR  p, LPCTSTR  pszkey)
{
	return 0;
}
/*****************************************************************************
* SetUpTuner
******************************************************************************
* Sets up global p_scanning_tuner and sets guid_network_type according
* to the Network Type requested.
*
* Logic: if tuner is set up and is the right network type, use it.
* Otherwise, poll the tuner for the right tuning space.
*
* Then set up a tune request and try to validate it. Finally, put
* tune request and tuning space to tuner
*
* on success, sets globals: p_scanning_tuner and guid_network_type
*
******************************************************************************/
HRESULT CBDAFilterGraph::SetUpTuner(REFCLSID guid_this_network_type)
{
	HRESULT hr = S_OK;
	class localComPtr
	{
	public:
		ITuningSpaceContainer*      p_tuning_space_container;
		IEnumTuningSpaces*          p_tuning_space_enum;
		ITuningSpace*               p_test_tuning_space;
		ITuneRequest*               p_tune_request;
		IDVBTuneRequest*            p_dvb_tune_request;

		IDigitalCableTuneRequest*   p_cqam_tune_request;
		IATSCChannelTuneRequest*    p_atsc_tune_request;
		ILocator*                   p_locator;
		IDVBTLocator*               p_dvbt_locator;
		IDVBCLocator*               p_dvbc_locator;
		IDVBSLocator*               p_dvbs_locator;

		BSTR                        bstr_name;

		CLSID                       guid_test_network_type;
		TCHAR*                       psz_network_name;
		TCHAR*                       psz_bstr_name;
		int                         i_name_len;

		localComPtr() :
			p_tuning_space_container(NULL),
			p_tuning_space_enum(NULL),
			p_test_tuning_space(NULL),
			p_tune_request(NULL),
			p_dvb_tune_request(NULL),
			p_cqam_tune_request(NULL),
			p_atsc_tune_request(NULL),
			p_locator(NULL),
			p_dvbt_locator(NULL),
			p_dvbc_locator(NULL),
			p_dvbs_locator(NULL),
			bstr_name(NULL),
			guid_test_network_type(GUID_NULL),
			psz_network_name(NULL),
			psz_bstr_name(NULL),
			i_name_len(0)
		{}
		~localComPtr()
		{
			if (p_tuning_space_enum)
				p_tuning_space_enum->Release();
			if (p_tuning_space_container)
				p_tuning_space_container->Release();
			if (p_test_tuning_space)
				p_test_tuning_space->Release();
			if (p_tune_request)
				p_tune_request->Release();
			if (p_dvb_tune_request)
				p_dvb_tune_request->Release();
			if (p_cqam_tune_request)
				p_cqam_tune_request->Release();
			if (p_atsc_tune_request)
				p_atsc_tune_request->Release();
			if (p_locator)
				p_locator->Release();
			if (p_dvbt_locator)
				p_dvbt_locator->Release();
			if (p_dvbc_locator)
				p_dvbc_locator->Release();
			if (p_dvbs_locator)
				p_dvbs_locator->Release();
			SysFreeString(bstr_name);
			delete[] psz_bstr_name;
			free(psz_network_name);
		}
	} l;

	msg_Dbg(p_access, TEXT("SetUpTuner: entering"));


	/* We shall test for a specific Tuning space name supplied on the command
	* line as dvb-network-name=xxx.
	* For some users with multiple cards and/or multiple networks this could
	* be useful. This allows us to reasonably safely apply updates to the
	* System Tuning Space in the registry without disrupting other streams. */

	l.psz_network_name = var_GetNonEmptyString(p_access, TEXT("dvb-network-name"));

	if (l.psz_network_name)
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: Find Tuning Space: %s"),
			l.psz_network_name);
	}
	else
	{
		l.psz_network_name = new TCHAR[1];
		*l.psz_network_name = '\0';
	}

	/* Tuner may already have been set up. If it is for the same
	* network type then all is well. Otherwise, reset the Tuning Space and get
	* a new one */
	msg_Dbg(p_access, TEXT("SetUpTuner: Checking for tuning space"));
	if (!p_scanning_tuner)
	{
		msg_Warn(p_access, TEXT("SetUpTuner: "\
			"Cannot find scanning tuner"));
		return E_FAIL;
	}

	if (p_tuning_space)
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: get network type"));
		hr = p_tuning_space->get__NetworkType(&l.guid_test_network_type);
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("Check: "\
				"Cannot get network type: hr=0x%8lx"), hr);
			l.guid_test_network_type = GUID_NULL;
		}

		msg_Dbg(p_access, TEXT("SetUpTuner: see if it's the right one"));
		if (l.guid_test_network_type == guid_this_network_type)
		{
			msg_Dbg(p_access, TEXT("SetUpTuner: it's the right one"));
			SysFreeString(l.bstr_name);

			hr = p_tuning_space->get_UniqueName(&l.bstr_name);
			if (FAILED(hr))
			{
				/* should never fail on a good tuning space */
				msg_Dbg(p_access, TEXT("SetUpTuner: "\
					"Cannot get UniqueName for Tuning Space: hr=0x%8lx"), hr);
				goto NoTuningSpace;
			}

			/* Test for a specific Tuning space name supplied on the command
			* line as dvb-network-name=xxx */
			if (l.psz_bstr_name)
				delete[] l.psz_bstr_name;
#ifdef UNICODE
			l.i_name_len = wcslen(l.bstr_name);
#else
			l.i_name_len = WideCharToMultiByte(CP_ACP, 0, l.bstr_name, -1,
				l.psz_bstr_name, 0, NULL, NULL);
#endif
			l.psz_bstr_name = new TCHAR[l.i_name_len];
#ifdef UNICODE
			wcscpy_s(l.bstr_name, l.i_name_len, l.psz_bstr_name);
#else
			l.i_name_len = WideCharToMultiByte(CP_ACP, 0, l.bstr_name, -1,
				l.psz_bstr_name, l.i_name_len, NULL, NULL);
#endif
			/* if no name was requested on command line, or if the name
			* requested equals the name of this space, we are OK */
			if (*l.psz_network_name == '\0' ||
				wcscmp(l.psz_network_name, l.psz_bstr_name) == 0)
			{
				msg_Dbg(p_access, TEXT("SetUpTuner: Using Tuning Space: %s"),
					l.psz_bstr_name);
				/* p_tuning_space and guid_network_type are already set */
				/* you probably already have a tune request, also */
				hr = p_scanning_tuner->get_TuneRequest(&l.p_tune_request);
				if (SUCCEEDED(hr))
				{
					return S_OK;
				}
				/* CreateTuneRequest adds l.p_tune_request to p_tuning_space
				* l.p_tune_request->RefCount = 1 */
				hr = p_tuning_space->CreateTuneRequest(&l.p_tune_request);
				if (SUCCEEDED(hr))
				{
					return S_OK;
				}
				msg_Warn(p_access, TEXT("SetUpTuner: "\
					"Cannot Create Tune Request: hr=0x%8lx"), hr);
				/* fall through to NoTuningSpace */
			}
		}

		/* else different guid_network_type */
	NoTuningSpace:
		if (p_tuning_space)
			p_tuning_space->Release();
		p_tuning_space = NULL;
		/* pro forma; should have returned S_OK if we created this */
		if (l.p_tune_request)
			l.p_tune_request->Release();
		l.p_tune_request = NULL;
	}


	/* p_tuning_space is null at this point; we have already
	returned S_OK if it was good. So find a tuning
	space on the scanning tuner. */

	msg_Dbg(p_access, TEXT("SetUpTuner: release TuningSpaces Enumerator"));
	if (l.p_tuning_space_enum)
		l.p_tuning_space_enum->Release();
	msg_Dbg(p_access, TEXT("SetUpTuner: nullify TuningSpaces Enumerator"));
	l.p_tuning_space_enum = NULL;
	msg_Dbg(p_access, TEXT("SetUpTuner: create TuningSpaces Enumerator"));

	hr = p_scanning_tuner->EnumTuningSpaces(&l.p_tuning_space_enum);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetUpTuner: "\
			"Cannot create TuningSpaces Enumerator: hr=0x%8lx"), hr);
		goto TryToClone;
	}

	do
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: top of loop"));
		l.guid_test_network_type = GUID_NULL;
		if (l.p_test_tuning_space)
			l.p_test_tuning_space->Release();
		l.p_test_tuning_space = NULL;
		if (p_tuning_space)
			p_tuning_space->Release();
		p_tuning_space = NULL;
		SysFreeString(l.bstr_name);
		msg_Dbg(p_access, TEXT("SetUpTuner: need good TS enum"));
		if (!l.p_tuning_space_enum) break;
		msg_Dbg(p_access, TEXT("SetUpTuner: next tuning space"));
		hr = l.p_tuning_space_enum->Next(1, &l.p_test_tuning_space, NULL);
		if (hr != S_OK) break;
		msg_Dbg(p_access, TEXT("SetUpTuner: get network type"));
		hr = l.p_test_tuning_space->get__NetworkType(&l.guid_test_network_type);
		if (FAILED(hr))
		{
			msg_Warn(p_access, TEXT("Check: "\
				"Cannot get network type: hr=0x%8lx"), hr);
			l.guid_test_network_type = GUID_NULL;
		}
		if (l.guid_test_network_type == guid_this_network_type)
		{
			msg_Dbg(p_access, TEXT("SetUpTuner: Found matching space on tuner"));

			SysFreeString(l.bstr_name);
			msg_Dbg(p_access, TEXT("SetUpTuner: get unique name"));

			hr = l.p_test_tuning_space->get_UniqueName(&l.bstr_name);
			if (FAILED(hr))
			{
				/* should never fail on a good tuning space */
				msg_Dbg(p_access, TEXT("SetUpTuner: "\
					"Cannot get UniqueName for Tuning Space: hr=0x%8lx"), hr);
				continue;
			}
			msg_Dbg(p_access, TEXT("SetUpTuner: convert w to multi"));
			if (l.psz_bstr_name)
				delete[] l.psz_bstr_name;
#ifdef UNICODE
			l.i_name_len = wcslen(l.bstr_name);
#else
			l.i_name_len = WideCharToMultiByte(CP_ACP, 0, l.bstr_name, -1,
				l.psz_bstr_name, 0, NULL, NULL);
#endif
			l.psz_bstr_name = new TCHAR[l.i_name_len];
#ifdef UNICODE
			wcscpy_s(l.bstr_name, l.i_name_len, l.psz_bstr_name);
#else
			l.i_name_len = WideCharToMultiByte(CP_ACP, 0, l.bstr_name, -1,
				l.psz_bstr_name, l.i_name_len, NULL, NULL);
#endif
			msg_Dbg(p_access, TEXT("SetUpTuner: Using Tuning Space: %s"),
				l.psz_bstr_name);
			break;
		}

	} while (true);
	msg_Dbg(p_access, TEXT("SetUpTuner: checking what we got"));

	if (l.guid_test_network_type == GUID_NULL)
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: got null, try to clone"));
		goto TryToClone;
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: put TS"));
	hr = p_scanning_tuner->put_TuningSpace(l.p_test_tuning_space);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: "\
			"cannot put tuning space: hr=0x%8lx"), hr);
		goto TryToClone;
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: get default locator"));
	hr = l.p_test_tuning_space->get_DefaultLocator(&l.p_locator);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: "\
			"cannot get default locator: hr=0x%8lx"), hr);
		goto TryToClone;
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: create tune request"));
	hr = l.p_test_tuning_space->CreateTuneRequest(&l.p_tune_request);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: "\
			"cannot create tune request: hr=0x%8lx"), hr);
		goto TryToClone;
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: put locator"));
	hr = l.p_tune_request->put_Locator(l.p_locator);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: "\
			"cannot put locator: hr=0x%8lx"), hr);
		goto TryToClone;
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: try to validate tune request"));
	hr = p_scanning_tuner->Validate(l.p_tune_request);
	if (FAILED(hr))
	{
		msg_Dbg(p_access, TEXT("SetUpTuner: "\
			"Tune Request cannot be validated: hr=0x%8lx"), hr);
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: Attach tune request to Scanning Tuner"));
	/* increments ref count for scanning tuner */
	hr = p_scanning_tuner->put_TuneRequest(l.p_tune_request);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("SetUpTuner: "\
			"Cannot submit the tune request: hr=0x%8lx"), hr);
		return hr;
	}

	msg_Dbg(p_access, TEXT("SetUpTuner: Tuning Space and Tune Request created"));
	return S_OK;

	/* Get the SystemTuningSpaces container
	* p_tuning_space_container->Refcount = 1  */
TryToClone:
	msg_Warn(p_access, TEXT("SetUpTuner: won't try to clone "));
	return E_FAIL;
}

HRESULT CBDAFilterGraph::GetFilterName(IBaseFilter* p_filter, char** psz_bstr_name)
{
	FILTER_INFO filter_info;
	HRESULT     hr = S_OK;

	hr = p_filter->QueryFilterInfo(&filter_info);
	if (FAILED(hr))
		return hr;
	int i_name_len = WideCharToMultiByte(CP_ACP, 0, filter_info.achName,
		-1, *psz_bstr_name, 0, NULL, NULL);
	*psz_bstr_name = new char[i_name_len];
	i_name_len = WideCharToMultiByte(CP_ACP, 0, filter_info.achName,
		-1, *psz_bstr_name, i_name_len, NULL, NULL);

	// The FILTER_INFO structure holds a pointer to the Filter Graph
	// Manager, with a reference count that must be released.
	if (filter_info.pGraph)
		filter_info.pGraph->Release();
	return S_OK;
}

HRESULT CBDAFilterGraph::GetPinName(IPin* p_pin, char** psz_bstr_name)
{
	PIN_INFO    pin_info;
	HRESULT     hr = S_OK;

	hr = p_pin->QueryPinInfo(&pin_info);
	if (FAILED(hr))
		return hr;
	int i_name_len = WideCharToMultiByte(CP_ACP, 0, pin_info.achName,
		-1, *psz_bstr_name, 0, NULL, NULL);
	*psz_bstr_name = new char[i_name_len];
	i_name_len = WideCharToMultiByte(CP_ACP, 0, pin_info.achName,
		-1, *psz_bstr_name, i_name_len, NULL, NULL);

	// The PIN_INFO structure holds a pointer to the Filter,
	// with a referenppce count that must be released.
	if (pin_info.pFilter)
		pin_info.pFilter->Release();
	return S_OK;
}

IPin* CBDAFilterGraph::FindPinOnFilter(IBaseFilter* pBaseFilter, const char* pPinName)
{
	HRESULT hr;
	IEnumPins *pEnumPin = NULL;
	ULONG CountReceived = 0;
	IPin *pPin = NULL, *pThePin = NULL;
	char String[80];
	char* pString;
	PIN_INFO PinInfo;
	int length;

	if (!pBaseFilter || !pPinName)
		return NULL;

	// enumerate of pins on the filter
	hr = pBaseFilter->EnumPins(&pEnumPin);
	if (hr == S_OK && pEnumPin)
	{
		pEnumPin->Reset();
		while (pEnumPin->Next(1, &pPin, &CountReceived) == S_OK && pPin)
		{
			memset(String, 0, sizeof(String));

			hr = pPin->QueryPinInfo(&PinInfo);
			if (hr == S_OK)
			{
				length = wcslen(PinInfo.achName) + 1;
				pString = new char[length];

				// get the pin name
				WideCharToMultiByte(CP_ACP, 0, PinInfo.achName, -1, pString, length,
					NULL, NULL);

				//strcat (String, pString);
				//StringCbCat(String,strlen(String) + strlen(pString)+1,pString);
				snprintf(String, strlen(String) + strlen(pString) + 1, "%s%s", String, pString);

				// is there a match
				if (strstr(String, pPinName))
					pThePin = pPin;   // yes
				else
					pPin = NULL;      // no

				delete[] pString;

			}
			else
			{
				// need to release this pin
				pPin->Release();
			}


		}        // end if have pin

				 // need to release the enumerator
		pEnumPin->Release();
	}

	// return address of pin if found on the filter
	return pThePin;

}

/*****************************************************************************
* Add/Remove a DirectShow filter graph to/from the Running Object Table.
* Allows GraphEdit to "spy" on a remote filter graph.
******************************************************************************/
HRESULT CBDAFilterGraph::Register()
{
	class localComPtr
	{
	public:
		IMoniker*             p_moniker;
		IRunningObjectTable*  p_ro_table;
		localComPtr() :
			p_moniker(NULL),
			p_ro_table(NULL)
		{};
		~localComPtr()
		{
			if (p_moniker)
				p_moniker->Release();
			if (p_ro_table)
				p_ro_table->Release();
		}
	} l;
	WCHAR     pwsz_graph_name[128];
	HRESULT   hr;

	hr = ::GetRunningObjectTable(0, &l.p_ro_table);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Register: Cannot get ROT: hr=0x%8lx"), hr);
		return hr;
	}

	size_t len = sizeof(pwsz_graph_name) / sizeof(pwsz_graph_name[0]);
	_snwprintf_s(pwsz_graph_name, len - 1, L"VLC BDA Graph %08x Pid %08x",
		(DWORD_PTR)p_filter_graph, ::GetCurrentProcessId());
	pwsz_graph_name[len - 1] = 0;
	hr = CreateItemMoniker(L"!", pwsz_graph_name, &l.p_moniker);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Register: Cannot Create Moniker: hr=0x%8lx"), hr);
		return hr;
	}
	hr = l.p_ro_table->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,
		p_filter_graph, l.p_moniker, &d_graph_register);
	if (FAILED(hr))
	{
		msg_Warn(p_access, TEXT("Register: Cannot Register Graph: hr=0x%8lx"), hr);
		return hr;
	}

	msg_Dbg(p_access, TEXT("Register: registered Graph: %S"), pwsz_graph_name);
	return hr;
}

void CBDAFilterGraph::Deregister()
{
	HRESULT   hr;
	IRunningObjectTable* p_ro_table;
	hr = ::GetRunningObjectTable(0, &p_ro_table);
	/* docs say this does a Release() on d_graph_register stuff */
	if (SUCCEEDED(hr))
		p_ro_table->Revoke(d_graph_register);
	d_graph_register = 0;
	p_ro_table->Release();
}


//
// USE THE CODE BELOW IF YOU WANT TO MANUALLY LOAD AND
// CONNECT A/V DECODERS TO THE DEMUX OUTPUT PINS
//

/*
To use this code:
1) in LoadAudioDecoder() and LoadVideoDecoder(), fill in decoder specific information (clsid)
2) goto BuildGraph() and replace RenderDemux() with BuildAVSegment()
*/

/*
// Builds the Audio, Video segment of the digital TV graph.
// Demux -> AV Decoder -> OVMixer -> Video Renderer
HRESULT
CBDAFilterGraph::BuildAVSegment()
{
    HRESULT hr = E_FAIL;

    // connect the demux to the capture device
    hr = ConnectFilters(m_pCaptureDevice, m_pDemux);

    hr = LoadVideoDecoder();

    if(SUCCEEDED(hr) && m_pVideoDecoder)
    {
        // Connect the demux & video decoder
        hr = ConnectFilters(m_pDemux, m_pVideoDecoder);

        if(FAILED(hr))
        {
            ErrorMessageBox("Connecting Demux & Video Decoder Failed\n");
            goto err;
        }
    }
    else
    {
        //ErrorMessageBox("Unable to load Video Decoder\n");
        goto err;
    }

    //Audio
    hr = LoadAudioDecoder();

    if(SUCCEEDED(hr) && m_pAudioDecoder)
    {
        hr = ConnectFilters(m_pDemux, m_pAudioDecoder);

        if(FAILED(hr))
        {
            ErrorMessageBox("Connecting Deumx & Audio Decoder Failed\n");
            goto err;
        }
    }
    else
    {
        ErrorMessageBox("Unable to load Audio Decoder\n");
        goto err;
    }

    // Create the OVMixer & Video Renderer for the video segment
    hr = CoCreateInstance(CLSID_OverlayMixer, NULL, CLSCTX_INPROC_SERVER,
            IID_IBaseFilter, reinterpret_cast<void**>(&m_pOVMixer));

    if(SUCCEEDED(hr) && m_pOVMixer)
    {
        hr = m_pFilterGraph->AddFilter(m_pOVMixer, L"OVMixer");

        if(FAILED(hr))
        {
            ErrorMessageBox("Adding OVMixer to the FilterGraph Failed\n");
            goto err;
        }
    }
    else
    {
        ErrorMessageBox("Loading OVMixer Failed\n");
        goto err;
    }

    hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
            IID_IBaseFilter, reinterpret_cast<void**>(&m_pVRenderer));

    if(SUCCEEDED(hr) && m_pVRenderer)
    {
        hr = m_pFilterGraph->AddFilter(m_pVRenderer, L"Video Renderer");

        if(FAILED(hr))
        {
            ErrorMessageBox("Adding Video Renderer to the FilterGraph Failed\n");
            goto err;
        }
    }
    else
    {
        ErrorMessageBox("Loading Video Renderer Failed\n");
        goto err;
    }

    // Split AV Decoder? Then add Default DirectSound Renderer to the filtergraph
    if(m_pVideoDecoder != m_pAudioDecoder)
    {
        hr = CoCreateInstance(CLSID_DSoundRender, NULL,
                        CLSCTX_INPROC_SERVER, IID_IBaseFilter,
                        reinterpret_cast<void**>(&m_pDDSRenderer));

        if(SUCCEEDED(hr) && m_pDDSRenderer)
        {
            hr = m_pFilterGraph->AddFilter(m_pDDSRenderer, L"Sound Renderer");

            if(FAILED(hr))
            {
                ErrorMessageBox("Adding DirectSound Device to the FilterGraph Failed\n");
                goto err;
            }
        }
        else
        {
            ErrorMessageBox("Loading DirectSound Device Failed\n");
            goto err;
        }
    }

    hr = ConnectFilters(m_pVideoDecoder, m_pOVMixer);

    if(FAILED(hr))
    {
        ErrorMessageBox("Connecting Capture & OVMixer Failed\n");
        goto err;
    }

    hr = ConnectFilters(m_pOVMixer, m_pVRenderer);

    if(FAILED(hr))
    {
        ErrorMessageBox("Connecting OVMixer & Video Renderer Failed\n");
        goto err;
    }

    // Split AV Decoder & if you need audio too ?? then connect Audio decoder to Sound Renderer
    if(m_pVideoDecoder != m_pAudioDecoder)
    {
        hr = ConnectFilters(m_pAudioDecoder, m_pDDSRenderer);

        if(FAILED(hr))
        {
            ErrorMessageBox("Connecting AudioDecoder & DirectSound Device Failed\n");
            goto err;
        }
    }

err:
    return hr;
}

// placeholders for real decoders
DEFINE_GUID(CLSID_FILL_IN_NAME_AUDIO_DECODER, 0xFEEDFEED, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00);
DEFINE_GUID(CLSID_FILL_IN_NAME_VIDEO_DECODER, 0xFEEDFEED, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00);

HRESULT
CBDAFilterGraph::LoadVideoDecoder()
{
    HRESULT hr = E_FAIL;

    hr = CoCreateInstance(CLSID_FILL_IN_NAME_VIDEO_DECODER, NULL,
            CLSCTX_INPROC_SERVER, IID_IBaseFilter,
            reinterpret_cast<void**>(&m_pVideoDecoder));

    if(SUCCEEDED(hr) && m_pVideoDecoder)
    {
        hr = m_pFilterGraph->AddFilter(m_pVideoDecoder, L"Video Decoder");

        if(FAILED(hr))
        {
            ErrorMessageBox("Unable to add Video Decoder filter to graph\n");
        }
    }

    return hr;
}


HRESULT
CBDAFilterGraph::LoadAudioDecoder()
{
    HRESULT hr = E_FAIL;

    hr = CoCreateInstance(CLSID_FILL_IN_NAME_AUDIO_DECODER, NULL,
            CLSCTX_INPROC_SERVER, IID_IBaseFilter,
            reinterpret_cast<void**>(&m_pAudioDecoder));

    if(SUCCEEDED(hr) && m_pAudioDecoder)
    {
        hr = m_pFilterGraph->AddFilter(m_pAudioDecoder, L"Audio Decoder");

        if(FAILED(hr))
        {
            ErrorMessageBox("Unable to add Audio filter to graph\n");
        }
    }

    return hr;
}

*/

