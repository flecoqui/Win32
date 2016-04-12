/*****************************************************************************
 * graph.h : DirectShow BDA graph for Win32 test
 *****************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
 //------------------------------------------------------------------------------
// File: Graph.h
//
// Desc: Sample code for BDA graph building.
//
// Copyright (c) 2000-2002, Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#ifndef GRAPH_H_INCLUDED_
#define GRAPH_H_INCLUDED_
//-----------------------------------------------------------------------------
// index of tuning spaces 
enum NETWORK_TYPE
{
	ATSC = 0x0003,
	CQAM = 0x00000002,

	DVB_C = 0x00000010,
	DVB_C2 = 0x00000020,
	DVB_S = 0x00000040,
	DVB_S2 = 0x00000080,
	DVB_T = 0x00000008,
	DVB_T2 = 0x00000200,
	ISDB_C = 0x00001000,
	ISDB_S = 0x00002000,
	ISDB_T = 0x00004000,
};

// Declaration of objects which are not exposed any more (qedit.h)
extern const CLSID CLSID_SampleGrabber; 
extern const IID IID_ISampleGrabber;
#define VLC_EGENERIC -1
#define VLC_SUCCESS 0

///////////////////////////////////////////////////////////////////////////////////

interface
	ISampleGrabberCB
	:
	public IUnknown
{
	virtual STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample) = 0;
	virtual STDMETHODIMP BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) = 0;
};

///////////////////////////////////////////////////////////////////////////////////



interface
	ISampleGrabber
	:
	public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback) = 0;
};


class MutexLocker
{
private:
	CRITICAL_SECTION *lock;
public:
	MutexLocker(CRITICAL_SECTION *m) : lock(m)
	{
		EnterCriticalSection(lock);
	}

	~MutexLocker(void)
	{
		LeaveCriticalSection(lock);
	}
};

struct BLOCK
{
	BLOCK    *p_next;

	BYTE    *p_buffer; /**< Payload start */
	size_t      i_buffer; /**< Payload length */
	BYTE    *p_start; /**< Buffer start */
	size_t      i_size; /**< Buffer total size */

	DWORD    i_flags;
	unsigned    i_nb_samples; /* Used for audio */
	/*
	mtime_t     i_pts;
	mtime_t     i_dts;
	mtime_t     i_length;

	*/
	void (* pf_release) (BLOCK *);
	
};
class BDAOutput
{
public:
	BDAOutput();
	~BDAOutput();

	void    Push(BLOCK *);
	LONG Pop(void *, ULONG);
	void    Empty();

private:
	CRITICAL_SECTION   lock;
	HANDLE    wait;
	BLOCK      *p_first;
	BLOCK     **pp_next;
};
/*
class ISampleGrabberCB : public IUnknown
{
public:
	virtual HRESULT __stdcall SampleCB(double d_sample_time,
		IMediaSample* p_sample) = 0;
	virtual HRESULT __stdcall BufferCB(double d_sample_time, BYTE *p_buffer,
		long l_bufferLen) = 0;
};
class ISampleGrabber : public IUnknown
{
public:
	virtual HRESULT __stdcall SetOneShot(BOOL b_one_shot) = 0;
	virtual HRESULT __stdcall SetMediaType(
		const AM_MEDIA_TYPE* p_media_type) = 0;
	virtual HRESULT __stdcall GetConnectedMediaType(
		AM_MEDIA_TYPE* p_media_type) = 0;
	virtual HRESULT __stdcall SetBufferSamples(BOOL b_buffer_samples) = 0;
	virtual HRESULT __stdcall GetCurrentBuffer(long* p_buff_size,
		long* p_buffer) = 0;
	virtual HRESULT __stdcall GetCurrentSample(IMediaSample** p_p_sample) = 0;
	virtual HRESULT __stdcall SetCallback(ISampleGrabberCB* pf_callback,
		long l_callback_type) = 0;
};

*/

class CBDAFilterGraph: public ISampleGrabberCB
{
private:
    CComPtr <ITuningSpace>   m_pITuningSpace;

    CComPtr <IScanningTuner> m_pITuner;

    CComPtr <IGraphBuilder>  m_pFilterGraph;         // for current graph
    CComPtr <IMediaControl>  m_pIMediaControl;       // for controlling graph state
    CComPtr <ICreateDevEnum> m_pICreateDevEnum;      // for enumerating system devices

    CComPtr <IBaseFilter>    m_pNetworkProvider;     // for network provider filter
    CComPtr <IBaseFilter>    m_pTunerDevice;         // for tuner device filter
    CComPtr <IBaseFilter>    m_pDemodulatorDevice;   // for tuner device filter
    CComPtr <IBaseFilter>    m_pCaptureDevice;       // for capture device filter
    CComPtr <IBaseFilter>    m_pDemux;               // for demux filter
    CComPtr <IBaseFilter>    m_pVideoDecoder;        // for mpeg video decoder filter
    CComPtr <IBaseFilter>    m_pAudioDecoder;        // for mpeg audio decoder filter
    CComPtr <IBaseFilter>    m_pTIF;                 // for transport information filter
    CComPtr <IBaseFilter>    m_pMPE;                 // for multiple protocol encapsulator
    CComPtr <IBaseFilter>    m_pIPSink;              // for ip sink filter
    CComPtr <IBaseFilter>    m_pOVMixer;             // for overlay mixer filter
    CComPtr <IBaseFilter>    m_pVRenderer;           // for video renderer filter
    CComPtr <IBaseFilter>    m_pDDSRenderer;         // for sound renderer filter
	CComPtr <IBaseFilter>    m_pSampleGrabber;       // for Sample grabbing
	ISampleGrabber*			 m_pGrabber;

    //required for an ATSC network when creating a tune request
    LONG                     m_lMajorChannel;
    LONG                     m_lMinorChannel;
    LONG                     m_lPhysicalChannel;

	// required for DVB Network when creating a tune request
	LONG m_frequency;
	LONG m_fec_hp;
	LONG m_fec_lp;
	LONG m_bandwidth;
	LONG m_transmission;
	LONG m_guard;
	LONG m_hierarchy;

    //registration number for the RunningObjectTable
    DWORD                    m_dwGraphRegister;



	
  
public:
    bool            m_fGraphBuilt;
    bool            m_fGraphRunning;
    bool            m_fGraphFailure;

    CBDAFilterGraph();   
    ~CBDAFilterGraph();




    
	size_t Pop(void *buf, size_t len);

    // Adds/removes a DirectShow filter graph from the Running Object Table,
    // allowing GraphEdit to "spy" on a remote filter graph if enabled.
    HRESULT AddGraphToRot(
        IUnknown *pUnkGraph, 
        DWORD *pdwRegister
        );

    void RemoveGraphFromRot(
        DWORD pdwRegister
        );

    LONG GetMajorChannel ()    { return m_lMajorChannel;    };
    LONG GetPhysicalChannel () { return m_lPhysicalChannel; };
    LONG GetMinorChannel ()    { return m_lMinorChannel;    };
	void msg_Warn(LPCTSTR  p, LPCTSTR  psz_format, ...);

	HRESULT GetName(LPWSTR pWName, LPSTR* psz_bstr_name);
	BDAOutput       output;
	bool bStop;
	bool bStopped;
	__int64 GlobalCounter;
	/**/
	/* registration number for the RunningObjectTable */
	DWORD     d_graph_register;
	CLSID     guid_network_type;   /* network type in use */

	ICreateDevEnum* p_system_dev_enum;
	IMediaControl*  p_media_control;
	IBaseFilter*    p_capture_device;
	ISampleGrabber* p_grabber;
	IBaseFilter*    p_mpeg_demux;
	IBaseFilter*    p_sample_grabber;
	IBaseFilter*    p_transport_info;
	LPCTSTR  p_access;
	IGraphBuilder*  p_filter_graph;
	IScanningTuner* p_scanning_tuner;
	ITuningSpace*               p_tuning_space;
	ITuneRequest*               p_tune_request;
	long      l_tuner_used;        /* Index of the Tuning Device in use */
	unsigned  systems;             /* bitmask of all tuners' network types */
	IBaseFilter*    p_network_provider;
	IBaseFilter*    p_tuner_device;
	unsigned GetSystem(REFCLSID clsid);
	LONG var_GetInteger(LPCTSTR  p, LPCTSTR  pszkey);
	LONG Receive(void* buf, LONG len);
	LPTSTR  var_GetNonEmptyString(LPCTSTR  p, LPCTSTR  pszkey);
	void msg_Dbg(LPCTSTR  p, LPCTSTR  psz_format, ...);
	HRESULT GetFilterName(IBaseFilter* p_filter, char** psz_bstr_name);
	HRESULT GetPinName(IPin* p_pin, char** psz_bstr_name);
	IPin* FindPinOnFilter(IBaseFilter* pBaseFilter, const char* pPinName);
	int SetDVBT(long l_frequency, unsigned __int32 fec_hp, unsigned __int32 fec_lp,
		long l_bandwidth, int transmission, unsigned __int32 guard, int hierarchy);
	HRESULT Build();
	HRESULT Check(REFCLSID guid_this_network_type);
	int SubmitTuneRequest(void);
	HRESULT Start();


	HRESULT SetUpTuner(REFCLSID guid_this_network_type);
	HRESULT Connect(IBaseFilter* p_filter_upstream,
		IBaseFilter* p_filter_downstream);
	HRESULT FindFilter(REFCLSID this_clsid, long* i_moniker_used,
		IBaseFilter* p_upstream, IBaseFilter** p_p_downstream);
	HRESULT Destroy();
	HRESULT Register();
	void Deregister();
	/**/


private:
	/* ISampleGrabberCB methods */
	ULONG ul_cbrc;
	STDMETHODIMP_(ULONG) AddRef() { return ++ul_cbrc; }
	STDMETHODIMP_(ULONG) Release() { return --ul_cbrc; }
	STDMETHODIMP QueryInterface(REFIID /*riid*/, void** /*p_p_object*/)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP SampleCB(double d_time, IMediaSample* p_sample);
	STDMETHODIMP BufferCB(double d_time, BYTE* p_buffer, long l_buffer_len);


 };
 
 
#endif // GRAPH_H_INCLUDED_
