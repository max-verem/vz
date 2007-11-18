/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2007 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


ChangeLog:
	2007-11-17
		*Starting minimal DirectShow source.

*/

#ifndef VZDSSRC_H
#define VZDSSRC_H

#ifdef _DEBUG
//#define TIME_DUMP_POINTS 15
#endif /* _DEBUG */

#define _CRT_SECURE_NO_WARNINGS
#define STRSAFE_NO_DEPRECATE
#define _WIN32_DCOM
#include <stdio.h>
#include <conio.h>

/* 
	BaseClasses - compile and comment:
	[...]
		#ifndef UNICODE
		//#define wsprintfW wsprintfWInternal
		//#define lstrcpyW lstrcpyWInternal
	[...]
*/
#include <streams.h>


/*
	How does the DEFINE_GUID macro work?
	http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/htm/directshowfaq.asp
*/
#include <initguid.h>

/* ------------------------------------------------------------------ */

#ifndef _DEBUG
#pragma comment(lib, "strmbase.lib")
//ATLS = atls.lib
//MSVCRT = msvcrt.lib
#else
#pragma comment(lib, "strmbasd.lib")
//ATLS = atlsd.lib
//MSVCRT = msvcrtd.lib
#endif

/* ------------------------------------------------------------------ */

// {506731F4-34B0-4123-8FC3-E5724D5E517B}
DEFINE_GUID(CLSID_VZPushSource, 
	0x506731f4, 0x34b0, 0x4123, 0x8f, 0xc3, 0xe5, 0x72, 0x4d, 0x5e, 0x51, 0x7b);

/* ------------------------------------------------------------------ */

class CVZPushPin : public CSourceStream
{
	long long sample_time;

#ifdef TIME_DUMP_POINTS
	unsigned long last_fill;
#endif /* TIME_DUMP_POINTS */

public:
	CVZPushPin(HRESULT *phr, CSource *pFilter);
    ~CVZPushPin();
    
	// We don't support any quality control.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    };

protected:
	HRESULT FillBuffer(IMediaSample *);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT GetMediaType(CMediaType *pMediaType);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT SetMediaType(const CMediaType *pMediaType);
};

/* ------------------------------------------------------------------ */

class CVZPushSource : public CSource
{

private:
public:
    // Constructor is private 
    // You have to use CreateInstance to create it.
    CVZPushSource(IUnknown *pUnk, HRESULT *phr);

public:
    static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);
};

/* ------------------------------------------------------------------ */

CVZPushSource::CVZPushSource(IUnknown *pUnk, HRESULT *phr): 
	CSource(NAME("VZPushSource"), pUnk, CLSID_VZPushSource)
{
    // Create the output pin.
    // The pin magically adds itself to the pin array.
    CVZPushPin *pPin = new CVZPushPin(phr, this);

    if (pPin == NULL)
    {
        *phr = E_OUTOFMEMORY;
    };
};

CUnknown * WINAPI CVZPushSource::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
    CVZPushSource *pNewFilter = new CVZPushSource(pUnk, phr );
    if (pNewFilter == NULL) 
    {
        *phr = E_OUTOFMEMORY;
    };
    return pNewFilter;
};
/* ------------------------------------------------------------------ */

CVZPushPin::CVZPushPin(HRESULT *phr, CSource *pFilter) :
	CSourceStream(NAME("VZPushPin"), phr, pFilter, L"Out")
{
	sample_time = 0;

#ifdef TIME_DUMP_POINTS
	last_fill = timeGetTime();
#endif /* TIME_DUMP_POINTS */
};

CVZPushPin::~CVZPushPin()
{

};

HRESULT CVZPushPin::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Pass the call up to my base class

	HRESULT hr = CSourceStream::SetMediaType(pMediaType);

	return hr;
}

HRESULT CVZPushPin::GetMediaType(CMediaType *pMediaType)
{
    CheckPointer(pMediaType, E_POINTER);

    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Allocate enough room for the VIDEOINFOHEADER and the color tables
    VIDEOINFOHEADER *pvi = (
		VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFO));

    if (pvi == 0) 
        return(E_OUTOFMEMORY);
	
	/* clear struct */
	ZeroMemory(pvi, sizeof(VIDEOINFO)); 
	
	/* setup framerate - in 100-nanosecond units*/
	pvi->AvgTimePerFrame = _tv->TV_FRAME_DUR_MS * 1000 * 10;

	/* setup dimensions and colur depth */
    BITMAPINFOHEADER *pBmi = &(pvi->bmiHeader);
    pBmi->biSize = sizeof(BITMAPINFOHEADER);
    pBmi->biWidth = _tv->TV_FRAME_WIDTH;
    pBmi->biHeight = _tv->TV_FRAME_HEIGHT;
    pBmi->biPlanes = 1;
    pBmi->biBitCount = 32; 

    // Set image size for use in FillBuffer
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);

    // Clear source and target rectangles
    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	/* set media types */
    pMediaType->SetType(&MEDIATYPE_Video);
    pMediaType->SetSubtype(&MEDIASUBTYPE_ARGB32);
    pMediaType->SetFormatType(&FORMAT_VideoInfo);
    pMediaType->SetTemporalCompression(FALSE);
    pMediaType->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return S_OK;
};

HRESULT CVZPushPin::CheckMediaType(const CMediaType *pMediaType)
{
	return 
		(
			(MEDIATYPE_Video == pMediaType->majortype)
			&&
			(
				(MEDIASUBTYPE_ARGB32 == pMediaType->subtype)
/*
				||
				(MEDIASUBTYPE_ARGB4444 == pMediaType->subtype)
*/
			)
		)? S_OK : E_FAIL;
};

HRESULT CVZPushPin::FillBuffer(IMediaSample *pSample)
{
    BYTE *pData;
    long cbData;
	void *output_buffer, **input_buffers, *output_a_buffer, **input_a_buffers;
//fprintf(stderr, "HERE2\n");
    CheckPointer(pSample, E_POINTER);

#ifdef TIME_DUMP_POINTS
	unsigned long last_fill2 = timeGetTime();
	unsigned long points[TIME_DUMP_POINTS];
	unsigned long points_index = 0;
#endif /* TIME_DUMP_POINTS */

    CAutoLock cAutoLock(m_pFilter->pStateLock());

#ifdef TIME_DUMP_POINTS
	points[points_index++] = timeGetTime();
#endif /* TIME_DUMP_POINTS */

	/* cast pointer to vzOutput */
	vzOutput* tbc = (vzOutput*)_tbc;

#ifdef TIME_DUMP_POINTS
	points[points_index++] = timeGetTime();
#endif /* TIME_DUMP_POINTS */

	/* request pointers to buffers */
	tbc->lock_io_bufs(&output_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

#ifdef TIME_DUMP_POINTS
	points[points_index++] = timeGetTime();
#endif /* TIME_DUMP_POINTS */

	/* Access the sample's data buffer */
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

#ifdef TIME_DUMP_POINTS
	points[points_index++] = timeGetTime();
#endif /* TIME_DUMP_POINTS */

	/* set datas */
	memcpy(pData, output_buffer, cbData);

#ifdef TIME_DUMP_POINTS
	points[points_index++] = timeGetTime();
#endif /* TIME_DUMP_POINTS */

	/* unlock buffers */
	tbc->unlock_io_bufs(&output_buffer, &input_buffers, &output_a_buffer, &input_a_buffers);

#ifdef TIME_DUMP_POINTS
	points[points_index++] = timeGetTime();
#endif /* TIME_DUMP_POINTS */

	/* Set TRUE on every sample for uncompressed frames */
    pSample->SetSyncPoint(TRUE);

	/* setup sample time */
	long long sample_time_current = sample_time;
	sample_time += _tv->TV_FRAME_DUR_MS * 1000 * 10;
	pSample->SetTime(&sample_time_current, &sample_time);

#ifdef TIME_DUMP_POINTS
	fprintf(stderr, "T: %5ld = ", last_fill2 - last_fill);
	for(int j = 0; j<points_index; j++)
		fprintf(stderr, " | %5ld", points[j] - last_fill2);
	fprintf(stderr, "\n");
	last_fill = last_fill2;
#endif /* TIME_DUMP_POINTS */

	/* send sync */
	if(_fc)
		_fc();

    return S_OK;
}

HRESULT CVZPushPin::DecideBufferSize(IMemAllocator *pAlloc, 
                                   ALLOCATOR_PROPERTIES *pRequest)
{
	HRESULT hr;

    CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (pRequest->cBuffers < 1)
    {
        pRequest->cBuffers = 1;  // We need at least one buffer
    };

	if (pRequest->cbBuffer < 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT)
	{
		pRequest->cbBuffer = 4*_tv->TV_FRAME_WIDTH*_tv->TV_FRAME_HEIGHT;
	};

	// Try to set these properties.
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pRequest, &Actual);

    if (FAILED(hr)) 
    {
        return hr;
    };

    // Check what we actually got. 

    if (Actual.cbBuffer < pRequest->cbBuffer) 
    {
        return E_FAIL;
    };

    return S_OK;
};

/* ------------------------------------------------------------------ */
#ifdef _DEBUG

/*
	"Loading a Graph From an External Process"
	http://msdn2.microsoft.com/en-us/library/ms787252.aspx
*/
static HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
    IMoniker * pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;

    if (FAILED(GetRunningObjectTable(0, &pROT))) 
    {
        return E_FAIL;
    }
    
    const size_t STRING_LENGTH = 256;

    WCHAR wsz[STRING_LENGTH];
    StringCchPrintfW(wsz, STRING_LENGTH, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());
    
    HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) 
    {
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph,
            pMoniker, pdwRegister);
        pMoniker->Release();
    }
    pROT->Release();
    
    return hr;
}

static void RemoveFromRot(DWORD pdwRegister)
{
    IRunningObjectTable *pROT;
    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
};

#endif

/* ------------------------------------------------------------------ */

/*
	http://msdn2.microsoft.com/en-us/library/ms787867.aspx
	http://msdn2.microsoft.com/en-us/library/ms782274.aspx
	http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c_Summer_03/directX/htm/connecttwofilters.asp
*/

static HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    IEnumPins  *pEnum;
    IPin       *pPin;
    pFilter->EnumPins(&pEnum);
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
        if (PinDir == PinDirThis)
        {
            pEnum->Release();
            *ppPin = pPin;
            return S_OK;
        }
        pPin->Release();
    }
    pEnum->Release();
    return E_FAIL;  
};

static HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pFirst, IBaseFilter *pSecond)
{
    IPin *pOut = NULL, *pIn = NULL;
    HRESULT hr = GetPin(pFirst, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr)) return hr;
    hr = GetPin(pSecond, PINDIR_INPUT, &pIn);
    if (FAILED(hr)) 
    {
        pOut->Release();
        return E_FAIL;
     }
    hr = pGraph->Connect(pOut, pIn);
    pIn->Release();
    pOut->Release();
    return hr;
};

#endif /* VZDSSRC_H */
