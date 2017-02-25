#pragma once
#include <string>
#include "g3log/logmessage.hpp"
#include "g3log/shared_queue.hpp"
#include "../../include/libuv/uv.h"


namespace g3 {
	class socketSink
	{
	public:
		socketSink(const std::string &svrIP, const unsigned int port, 
			const std::string &logger_id = "g3log");
		~socketSink();

		socketSink &operator=(const socketSink &) = delete;
		socketSink(const socketSink &other) = delete;

		void sendMessage(LogMessageMover message);

		// inner send
		void send();
		int libuvThread();

	private:
		uv_thread_t		_thread;
		uv_cond_t		_prepareCond;
		uv_mutex_t		_prepareMutex;

		uv_async_t		*_uvAsync;
		uv_loop_t		*_uvLoop;
		uv_tcp_t		*_uvClient;

		std::string		_svrIP;
		unsigned int	_svrPort;

		shared_queue<LogMessage> _logMQ;
	};
}



