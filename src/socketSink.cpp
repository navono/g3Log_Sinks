#include "g3log/socketSink.h"

namespace g3
{
	// TODO: need check libuv function return code!


	void after_write(uv_write_t* req, int status)
	{
		free(req);
	}

	void on_read_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		buf->base = (char*)malloc(suggested_size);
		buf->len = suggested_size;
	}

	void on_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
	{
		// not process data from server
		free(buf->base);
	}

	void on_async_cb(uv_async_t* handle)
	{
		socketSink *sink = static_cast<socketSink*>(handle->data);
		if (sink != nullptr)
		{
			sink->send();
		}
	}

	void on_uv_connect(uv_connect_t* req, int status)
	{
		if (status < 0)
		{
			// error
			auto str = uv_err_name(status);
			return;
		}

		auto r = uv_read_start((uv_stream_t*)req->handle, on_read_alloc, on_read_cb);
		auto s = uv_strerror(r);
	}

	void thread_worker(void* param)
	{
		socketSink *sink = static_cast<socketSink*>(param);
		if (sink != nullptr)
		{
			sink->libuvThread();
		}

		return;
	}




	socketSink::socketSink(const std::string &svrIP, const unsigned int port, 
		const std::string &logger_id /*= "g3log"*/)
		: _svrIP(svrIP)
		, _svrPort(port)
		, _uvLoop(nullptr)
		, _uvClient(nullptr)
		, _uvAsync(nullptr)
	{
		uv_cond_init(&_prepareCond);
		uv_mutex_init(&_prepareMutex);

		uv_thread_create(&_thread, thread_worker, this);

		// 等待libuv线程创建链路，因为来自于客户端的log和libuv的log发送都是异步的
		// 而客户端的log发送应该等到libuv环境都准备好后才能执行
		uv_mutex_lock(&_prepareMutex);
		uv_cond_wait(&_prepareCond, &_prepareMutex);
		uv_mutex_unlock(&_prepareMutex);
	}

	socketSink::~socketSink()
	{
		uv_stop(_uvLoop);

		// 等待libuv线程退出
		uv_mutex_lock(&_prepareMutex);
		uv_cond_wait(&_prepareCond, &_prepareMutex);
		uv_mutex_unlock(&_prepareMutex);

		uv_close((uv_handle_t*)_uvClient, nullptr);
		uv_close((uv_handle_t*)_uvAsync, nullptr);
		uv_loop_close(_uvLoop);

		// 不再接收回调关闭，再次直接free
		free(_uvLoop);
		free(_uvClient);
		free(_uvAsync);
	}

	void socketSink::sendMessage(LogMessageMover message)
	{
		_logMQ.push(message.get());
		uv_async_send(_uvAsync);
	}

	void socketSink::send()
	{
		LogMessage msg("");
		while (_logMQ.try_and_pop(msg))
		{
			auto logString = msg.toString();
			uv_buf_t logBuf = uv_buf_init((char*)logString.c_str(), logString.size());

			uv_write_t *wq = (uv_write_t*)malloc(sizeof uv_write_t);
			uv_write(wq, (uv_stream_t*)_uvClient, &logBuf, 1, after_write);
		}
	}

	int socketSink::libuvThread()
	{
		uv_mutex_lock(&_prepareMutex);

		_uvLoop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		_uvClient = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
		_uvAsync = (uv_async_t *)malloc(sizeof(uv_async_t));

		auto r = uv_loop_init(_uvLoop);
		r = uv_tcp_init(_uvLoop, _uvClient);

		r = uv_async_init(_uvLoop, _uvAsync, on_async_cb);
		_uvAsync->data = this;

		sockaddr_in svrAddr;
		uv_ip4_addr(_svrIP.c_str(), _svrPort, &svrAddr);

		uv_connect_t uvCon;
		r = uv_tcp_connect(&uvCon, _uvClient, (struct sockaddr*)&svrAddr, on_uv_connect);

		// 下面的signal是为了告知构造函数，链路已经准备好，可以接收消息了
		uv_mutex_unlock(&_prepareMutex);
		uv_cond_signal(&_prepareCond);
		
		uv_run(_uvLoop, UV_RUN_DEFAULT);

		// 这里的signal是为了告知析构函数，libuv线程退出
		uv_cond_signal(&_prepareCond);
		return 1;
	}
}