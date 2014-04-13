/*
Copyright (c) 2014 MVPETE

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef __NPP_SHARED_H__
#define __NPP_SHARED_H__
#include <Windows.h>

#include <stdexcept>

/// define functions
#define FN_WRITE_X 0x01
#define FN_WRITE_S 0x02

#define FN_READ_X  0x03
#define FN_READ_S  0x04


////////////////////////////////////////////////////////////////////////////////////
/// io_event - wraps Windows event, allows a timeoutable wait
class io_event
{
	HANDLE event_;

public:
	io_event(bool auto_reset=FALSE)
	{
		event_ = ::CreateEvent(NULL,!auto_reset,FALSE,NULL);
		if( event_ == NULL )
			throw std::runtime_error("failed to initialize event");
	}
	~io_event()
	{
		::CloseHandle(event_);
	}

	bool wait(int timeout)
	{
		DWORD res = ::WaitForSingleObject(event_,timeout);
		if( res != WAIT_OBJECT_0 ) // wait object set, otherwise timed out
			return false;
		return true;
	}

	void set() { ::SetEvent(event_); }
	void reset() { ::ResetEvent(event_); }

	HANDLE handle() { return event_; }
};

#endif // __NPP_SHARED_H__
