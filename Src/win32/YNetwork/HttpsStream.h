#pragma once

#include <atomic>
#include <thread>
#include <string>
#include <algorithm>
#include <functional>
#include <sstream>

#include "include/QAQNetwork.h"
class HttpStream :public QAQNetwork::Delegate
{

public:
	HttpStream()
	{
		CreateNetwork(&network_, this);
	}
	~HttpStream()
	{
		DeleteNetwork(&network_);
	}

	QAQNetwork* getNetwork()
	{
		return network_;
	}

	void Conduct()
	{
		is_stop_ = true;
	}

	void Stop()
	{
		network_->stopAsync();
		clearMemData();
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
		int64_t download_len = getMemSize();
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

			download_len = getMemSize();
		}

		int nbytes = (int64_t)std::min<int>(download_len - cur_pos_, len);
		if (nbytes <= 0) {
			return -1;
		}

		char * data_src = (char *)getMemData();
		memcpy_s(data, (int)nbytes, data_src + cur_pos_, (int)nbytes);

		cur_pos_ += nbytes;
		return nbytes;
	}

	int64_t Seek(int64_t offset, int whence)
	{
		int64_t download_len = getMemSize();
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
	std::atomic_int cur_pos_ = 0;
	int reply_error_ = 0;
	std::atomic_int file_len_=0;
	std::function<void (float)> buffer_func_=nullptr;
	std::function<void ()> status_buffering_func_ = nullptr;
	

	QAQNetwork *network_ = nullptr;
	

	int64_t GetFileLength()
	{
		return file_len_;
	}
	int64_t getMemSize()
	{
		std::lock_guard<std::mutex> lg(mutex_);
		return stream_.size();
	}
	const char * getMemData()
	{
		std::lock_guard<std::mutex> lg(mutex_);
		return stream_.c_str();
	}
	void clearMemData()
	{
		std::lock_guard<std::mutex> lg(mutex_);
		stream_ = "";
	}
	virtual bool onTrunk(QAQNetwork *, char *data, int len) override
	{
		std::lock_guard<std::mutex> lg(mutex_);
		stream_.append(data, len);
		return true;
	}


	virtual void onReply(QAQNetwork *, QAQNetworkReply* reply) override
	{
		if (reply->getError() != QAQNetworkReplyError::REPLY_NO_ERROR)
		{
			reply_error_ = 1;
		}
	}


	virtual void onDataProgress(QAQNetwork *, double total, double now) override
	{
		file_len_ = total;
		if (buffer_func_)
		{
			buffer_func_((float)now / total);
		}
	}
	
	std::mutex mutex_;
	std::string stream_;
};