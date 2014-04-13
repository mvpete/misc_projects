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

#include <string>
#include <sstream>
#include <iostream>
////////////////////////////////////////////////////////////////////////////////////
/// npp_server - named pipe server, encapsulates the Windows named pipe creation,
/// reading and writing.
/// important - keep in mind the name format
class npp_server
{
	HANDLE pipe_;

public:
	// name format - "\\\\.\\pipe\\comm_pipe"
	npp_server(const char *pipe_name, int clients_allowed)
	{
		pipe_ = ::CreateNamedPipe(pipe_name,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE | PIPE_WAIT,
			clients_allowed,
			512, // 512 byte buffer out
			512, // 512 byte buffer in
			5000, // 5 second timeout
			NULL);

		if( pipe_ == INVALID_HANDLE_VALUE )
		{
			std::stringstream ss;
			ss << "failed to create pipe server - " << ::GetLastError();
			throw std::runtime_error(ss.str());
		}

	}
	void disconnect() { ::DisconnectNamedPipe(pipe_); }
	~npp_server()
	{
		
		::CloseHandle(pipe_);
	}

	/// waits for a client to connect, or timeout
	bool wait_for_client(int timeout)
	{
		OVERLAPPED io;
		memset(&io,0,sizeof(OVERLAPPED));
		io_event ioe;
		io.hEvent=ioe.handle();
		if( !::ConnectNamedPipe(pipe_,&io) )
		{
			DWORD res = ::GetLastError();
			if(res == ERROR_IO_PENDING)
			{
				return ioe.wait(timeout);
			}
			else
				return false;
		}
		return true;
	}

	int write(const char *buffer, int size, int timeout)
	{
		io_event ioe;

		OVERLAPPED io;
		memset(&io,0, sizeof(OVERLAPPED));

		io.hEvent = ioe.handle();

		DWORD bwritten(0);

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

	int read(char *buffer, int offset, int size, int timeout)
	{
		io_event ioe;
		OVERLAPPED io;
		memset(&io,0,sizeof(OVERLAPPED));
		io.hEvent = ioe.handle();

		DWORD bread(0);
		if(!::ReadFile(pipe_,buffer+offset,size,&bread,&io))
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
			else if (res == ERROR_BROKEN_PIPE)
				return 0; // pipe closed
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
/* 
	ssp_parser - parser class for the SSP protocol, only supports two functions
	to write a string and write an integer value.

	usage - supply named pipe server and callbacks implementation, named pipe
	server must already be setup and connected to client.

*/
class ssp_parser
{
	
	npp_server &srv_;
	char rbuf_[512];
	int size_;

	void process_buffer()
	{
		while(size_ > 3)
		{
			unsigned char msg_size = rbuf_[3];
			if( size_ < msg_size )
				return;
		
			unsigned char fc = rbuf_[4];
			switch(fc)
			{
			case FN_WRITE_X:
				cb_.on_write_x(*(int*)(rbuf_+5));
				break;
			case FN_WRITE_S:
				char *buf = new char[msg_size-5];
				memcpy(buf,rbuf_+5,msg_size-5);
				cb_.on_write_s(buf);
				delete []buf;
				break;
			}

			// pop off the msg 
			const char *offset_ptr=rbuf_+msg_size;
			int cpyout = size_-msg_size;
			memcpy(rbuf_,offset_ptr, cpyout);
			size_-= msg_size;
		}
	}

public:
	class callbacks
	{
	public:
		virtual ~callbacks(){};
		virtual void on_write_x(int value) =0;
		virtual void on_write_s(const char *str) =0;

		//virtual void on_read_x(ssp_parser *rsp)=0;
		//virtual void on_read_s(ssp_parser *rsp)=0;

	};


	ssp_parser(npp_server &srv, callbacks &cb):srv_(srv),cb_(cb),size_(0){}

	void send_read_x_rsp(int value);
	void send_read_s_rsp(const char *value);


	void run(io_event &ioe)
	{
		while(!ioe.wait(0)) // running
		{
			int read = srv_.read(rbuf_,size_, 512-size_,INFINITE);
			if (read == 0)
				return;
			size_ += read;
			if( size_ < 4 ) // read header and length
				continue;

			process_buffer();
		}
	}

private:
	callbacks &cb_;

};


class marshaller : public ssp_parser::callbacks
{
	int x_;
	std::string s_;
public:

	void on_write_x(int value) 
	{
		x_ = value;
		std::cout << "Read value: " << value << std::endl;
	}
	void on_write_s(const char *str)
	{
		s_ = std::string(str);
		std::cout << "Read string: " << s_ << std::endl;
	}

	int get_xval() { return x_; }
	std::string& get_string() { return s_; }
};

#include <conio.h>
io_event running;
DWORD WINAPI runner(LPVOID params)
{
	std::cout << "Press ENTER to stop server." << std::endl;
	_getch();
	running.set();
	return 0;
}

int main()
{
	npp_server srv("\\\\.\\pipe\\npp_test", 1);
	std::cout << "named pipe server - \\\\.\\pipe\\npp_test" << std::endl;
	

	marshaller cb; // callbacks marshaller

	ssp_parser parser(srv,cb);
	DWORD threadid;
	HANDLE run = ::CreateThread(NULL,0,runner,0,0,&threadid);
	while(!running.wait(0))
	{
		if(!srv.wait_for_client(1000))// try to connect a client, otherwise check if we're stopped and try again
			continue; 
		std::cout << "client connected" << std::endl;
		parser.run(running); // will block until client disconnects
		srv.disconnect(); // disconnect that client
		std::cout << "client disconnected" << std::endl;
		
	}

	::WaitForSingleObject(run,INFINITE); // wait for the thread to clean up

}
