/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2011 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2011.

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
*/
#ifndef decklink_input_class_h
#define decklink_input_class_h

class decklink_input_class: public IDeckLinkInputCallback
{
    int m_RefCount;
    int m_idx;
    decklink_runtime_context_t* m_pController;

public:

    decklink_input_class (decklink_runtime_context_t* pController, int idx)
    {
        m_pController = pController;
        m_idx = idx;
        m_RefCount = 1;
    };

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
    {
        HRESULT result = E_NOINTERFACE;

        // Initialise the return result
        *ppv = NULL;

        // Obtain the IUnknown interface and compare it the provided REFIID
        if (iid == IID_IUnknown)
        {
            *ppv = this;
            AddRef();
            result = S_OK;
        }
        else if (iid == IID_IDeckLinkInputCallback)
        {
            *ppv = (IDeckLinkInputCallback*)this;
            AddRef();
            result = S_OK;
        }

        return result;
    };

    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return InterlockedIncrement((LONG*)&m_RefCount);
    };

    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        int newRefValue;

        newRefValue = InterlockedDecrement((LONG*)&m_RefCount);
        if(newRefValue == 0)
        {
            delete this;
            return 0;
        }

        return newRefValue;
    };

    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged
        (BMDVideoInputFormatChangedEvents notificationEvents,
        IDeckLinkDisplayMode* newDisplayMode,
        BMDDetectedVideoInputFormatFlags detectedSignalFlags)
    {
        decklink_VideoInputFormatChanged(m_pController, m_idx,
            notificationEvents, newDisplayMode, detectedSignalFlags);
        return S_OK;
    };

    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived
        (IDeckLinkVideoInputFrame* pArrivedFrame,
        IDeckLinkAudioInputPacket* pAudio)
    {
        decklink_VideoInputFrameArrived(m_pController, m_idx,
            pArrivedFrame, pAudio);
        return S_OK;
    };
};

#endif /* decklink_input_class_h */
