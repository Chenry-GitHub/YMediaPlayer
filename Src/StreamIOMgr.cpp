#include "StreamIOMgr.h"
#include <corecrt_io.h>




StreamIOMgr::StreamIOMgr()
{
	
}


StreamIOMgr::~StreamIOMgr()
{
}


void StreamIOMgr::Conduct()
{
	switch (stream_type_)
	{
	case ST_UNKNOWN:
	{
		http_stream_.Conduct();
	}
		break;
	case ST_FILE:
	{

	}
		break;
	case ST_HTTP:
	{
		http_stream_.Conduct();
	}
		break;
	default:
		break;
	}
}

void StreamIOMgr::Stop()
{
	switch (stream_type_)
	{
	case ST_UNKNOWN:
	{
		if (file_stream_)
		{
			fclose(file_stream_);
			file_stream_ = nullptr;
		}
		http_stream_.Stop();
	}
		break;
	case ST_FILE:
	{
		if (file_stream_)
		{
			fclose(file_stream_);
			file_stream_ = nullptr;
		}
	}
		break;
	case ST_HTTP:
	{
		http_stream_.Stop();
	}
		break;
	default:
		break;
	}
	is_stop_ = true;
}

bool StreamIOMgr::SetUrl(const std::string &url)
{
	std::string temp = url;
	std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
	if (temp.find("http://") != std::string::npos ||
		temp.find("https://") != std::string::npos)
	{
		http_stream_.buffer_func_ = buffer_func_;
		http_stream_.status_buffering_func_ = std::bind([&]() {
			if (status_func_)
			{
				status_func_(PlayerStatus::Buffering);
			}
		});
		http_stream_.error_func_ = std::bind([&]() {
			if (status_func_)
			{
				status_func_(PlayerStatus::ErrorUnknow);
			}
		});

		http_stream_.Start();
		http_stream_.GetNetworkRequest()->SetUrl(url.c_str());
		http_stream_.GetNetwork()->ASyncGet2(http_stream_.GetNetworkRequest(), &http_stream_);
		stream_type_ = ST_HTTP;
	}
	else if (_access(url.c_str(),0) == 0)
	{			
		file_stream_= fopen(url.c_str(), "rb");
		stream_type_ = ST_FILE;
	}
	else
	{
		stream_type_ = ST_UNKNOWN;
	}
	return true;
}

int StreamIOMgr::Read(char *data, int len)
{
	int read_size = 0;
	switch (stream_type_)
	{
	case ST_UNKNOWN:
		break;
	case ST_FILE:
	{
		read_size = fread(data, 1, len, file_stream_);
	}
		break;
	case ST_HTTP:
	{
		read_size  = http_stream_.Read(data, len);
	}
		break;
	default:
		break;
	}
	return read_size;
}

int64_t StreamIOMgr::Seek(int64_t offset, int whence)
{
	int64_t pos =0;
	switch (stream_type_)
	{
	case ST_UNKNOWN:
		break;
	case ST_FILE:
	{
		if (whence == 0x10000)
		{
			return -1;
		}
		fseek(file_stream_, offset, whence);
		pos = ftell(file_stream_);
	}
		break;
	case ST_HTTP:
	{
		pos = http_stream_.Seek(offset, whence);
	}
		break;
	default:
		break;
	}
	return pos;
}
