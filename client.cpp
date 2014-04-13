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



#include <shared.h>

#include <sstream>

////////////////////////////////////////////////////////////////////////////////////
/// npp_client - a wrapper for Windows named pipe for communication
class npp_client
{
	HANDLE pipe_;

public:

	npp_client(const char *pipe_name)
	{
		// open the connection to the named pipe
		pipe_ = ::CreateFile(pipe_name,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if( pipe_ == INVALID_HANDLE_VALUE )
		{
			std::stringstream ss;
			ss << "failed to initialized pipe connection - " << ::GetLastError();
			throw std::runtime_error(ss.str());
		}
	}
	~npp_client()
	{
		::CloseHandle(pipe_);
	}

	
	int write(const unsigned char *buffer, int size, int timeout)
	{
		io_event ioe;

		OVERLAPPED io;
		memset(&io,0, sizeof(OVERLAPPED));

		io.hEvent = ioe.handle();

		DWORD bwritten(0);
		/// write to the pipe, wait if nescessary
		if(!::WriteFile(pipe_,buffer,size,&bwritten,&io))
		{
			DWORD res = ::GetLastError();
			if( res == ERROR_IO_PENDING )
			{
				// this will block on the wait until it's triggered
				if(ioe.wait(timeout) && ::GetOverlappedResult(pipe_,&io,&bwritten,false))
					return bwritten;
				else
				{
					::CancelIo(pipe_);
				}
			}
			else
			{
				std::stringstream ss;
				ss << "Failed to write to stream - " <<::GetLastError();
				throw std::runtime_error(ss.str());
			}
		}
		return bwritten;
	}

	int read(unsigned char *buffer, int size, int timeout)
	{
		io_event ioe;
		OVERLAPPED io;
		memset(&io,0,sizeof(OVERLAPPED));
		io.hEvent = ioe.handle();

		DWORD bread(0);
		// read the pipe, wait timeout if nescesaary
		if(!::ReadFile(pipe_,buffer,size,&bread,&io))
		{
			DWORD res = ::GetLastError();
			if( res == ERROR_IO_PENDING )
			{
				// this will block on the wait until it's triggered
				if(ioe.wait(timeout) && ::GetOverlappedResult(pipe_,&io,&bread,false))
					return bread;
				else
				{
					::CancelIo(pipe_);
				}
			}
			else
			{
				std::stringstream ss;
				ss << "Failed to read from stream - " <<::GetLastError();
				throw std::runtime_error(ss.str());
			}
		}
		return bread;
	}

};

////////////////////////////////////////////////////////////////////////////////////
/// message - a message builder to make it convient to build buffers
/// warning - no checks on buffer size
class message
{
	unsigned char buf_[128];
	int  size_;
	unsigned char &msg_size_;
	unsigned char &fc_;

	void commit(int bytes);

public:

	message():
	msg_size_(buf_[3]),
	fc_(buf_[4])
	{
		memset(buf_,0,128);
		memcpy(buf_,"SSP",3);
		size_=5;
	}


	void function(unsigned char fc)
	{
		fc_=fc;
	}

	void push_back(const char *s, int cnt)
	{
		if(size_ + cnt >= 128)
			throw std::runtime_error("overflow");

		memcpy(&buf_[size_],s,cnt);
		size_+=cnt;
	}
	void push_back(const char *str)
	{
		push_back(str,strlen(str)+1);
	}

	void push_back(int val)
	{
		if(size_ +sizeof(int) >= 128)
			throw std::runtime_error("overflow");
		memcpy(&buf_[size_],&val,sizeof(int));
		size_+=sizeof(int);
	}

	void package()
	{
		msg_size_=size_;
	}

	const unsigned char* c_ptr() { return buf_; }

	int size() { return size_; }
};


#include <iostream>
#include <conio.h>
int main()
{
	npp_client client("\\\\.\\pipe\\npp_test");

	message m1; // create a msg buffer
	m1.function(FN_WRITE_S); // set the function code to tell the server to write s
	m1.push_back("HELLO WORLD"); // put the data in
	m1.package(); // package the message
	client.write(m1.c_ptr(),m1.size(), INFINITE);

	message m2;
	m2.function(FN_WRITE_X);
	m2.push_back(15);
	m2.package();
	client.write(m2.c_ptr(),m2.size(), INFINITE);

	std::string str;
	while(str.compare("quit") != 0)
	{
		try
		{
			std::cout << "enter a string to send (quit to exit)";
			std::getline(std::cin,str);
			message m;
			m.function(FN_WRITE_S);
			m.push_back(str.c_str());
			m.package();
			client.write(m.c_ptr(),m.size(), INFINITE);
		}
		catch(std::exception &e)
		{
			std::cout << "error - " << e.what() << std::endl;
			str="";
		}
	}

	
}
