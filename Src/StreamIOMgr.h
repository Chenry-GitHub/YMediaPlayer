#pragma once
#include <string>
#include <mutex>
#include <algorithm>
#include <functional>
#include <fstream>
#include <atomic>
#include "YMediaComm.h"
#include "win32/YNetwork/HttpsStream.h"

enum StreamType
{
	ST_UNKNOWN,
	ST_FILE,
	ST_HTTP,
};
class StreamIOMgr
{
public:
	StreamIOMgr();
	~StreamIOMgr();

	void Conduct();

	void Stop();

	bool SetUrl(const std::string &url);

	int Read(char *data,int len);

	int64_t Seek(int64_t offset ,int whence);

	std::function <void(float)> buffer_func_ = nullptr;

	std::function <void (PlayerStatus)> status_func_ = nullptr;

protected:

	//file stream
	FILE* file_stream_=nullptr;
	//network HTTP
	HttpStream http_stream_;
	StreamType stream_type_;


	std::atomic_bool is_stop_ = true;
};

