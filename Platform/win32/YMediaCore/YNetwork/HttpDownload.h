#pragma once

#include "BaseHttpASync.h"
#include <atomic>
#include <thread>
#include <string>
#include <algorithm>
#include <functional>
class HttpDownload :public BaseHttpAsync
{

public:
	~HttpDownload()
	{
	
	}
	virtual bool OnAnalysis(const std::string &data) override
	{
		file_len_ = data.size();
		if (data.size() == 0)
			return false;
		return true;
	}


	virtual void OnAfterHandleReply(bool success) override
	{
		if (!success)
		{
			reply_error_ = 1;
		}	
	}
	void Conduct()
	{
		is_stop_ = true;
	}

	void Stop()
	{
		GetNetwork()->Stop();
		is_stop_ = true;
		reply_error_ = 0;
		cur_pos_ = 0;
		file_len_ = 0;
	}

	void Start()
	{
		is_stop_ = false;
	}

	int Read(char *data, int len)
	{
		int64_t download_len = GetNetwork()->GetMemorySize();
		while (download_len < len + cur_pos_ && cur_pos_ + len < file_len_ || file_len_ <= 0)
		{
			if (file_len_ == download_len && file_len_ > 0)
				break;

			if (status_buffering_func_)
				status_buffering_func_();
			
			if (is_stop_)
			{
				return -3; //RE_USER_INTERRUPT , user interrupt
			}
			else if (reply_error_)
			{
				return -2; //RE_TIMEOUT , time out
			}
			std::this_thread::sleep_for(std::chrono::microseconds(10));

			download_len = GetNetwork()->GetMemorySize();

		}

		int nbytes = (int64_t)std::min<int>(download_len - cur_pos_, len);
		if (nbytes <= 0) {
			return -1;
		}

		char * data_src = (char *)GetNetwork()->GetMemoryData();
		memcpy_s(data, (int)nbytes, data_src + cur_pos_, (int)nbytes);

		cur_pos_ += nbytes;
		return nbytes;
	}

	int64_t Seek(int64_t offset, int whence)
	{
		int64_t download_len = GetNetwork()->GetMemorySize();
		int64_t newPos = -1;
		switch (whence) {
		case SEEK_SET: newPos = offset; break;
		case SEEK_CUR: newPos = cur_pos_ + offset; break;
		case SEEK_END: newPos = download_len + offset; break;
		case 0x10000: {
			// Special whence for determining filesize without any seek.
			return file_len_;
		} break;
		}
		if (newPos < 0 || newPos > download_len) {
			return -1;
		}
		cur_pos_ = newPos;
		return cur_pos_;
	}

	std::atomic_bool is_stop_;
	int cur_pos_ = 0;
	int reply_error_ = 0;
	std::atomic_int file_len_=0;
	std::function<void (float)> buffer_func_=nullptr;
	std::function<void ()> status_buffering_func_ = nullptr;
	//std::function<void (bool)> error_func_=nullptr;
	
	virtual void OnDataProgress(double total, double now) override
	{
		file_len_ = total;
		if (buffer_func_)
		{
			buffer_func_((float)now/total);
		}
	}

	int64_t GetFileLength()
	{
		return file_len_;
	}
};