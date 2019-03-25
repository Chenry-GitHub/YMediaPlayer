#pragma once
/*
@author:yantao
@email:what951006@163.com
@time:2019-03-09 16:10:37
@brief:supported synchronized and asynchronous to get result
*/

#ifndef QAQNETWORK_STATIC
	#ifdef QAQNETWORK_EXPORT
	#define QAQNETWORK_LIB _declspec(dllexport)
	#else
	#define QAQNETWORK_LIB _declspec(dllimport)
	#endif
#else
	#define QAQNETWORK_LIB
#endif

class QAQNetworkReq;
class QAQNetworkReply;

using DataCallBack = void(*)(QAQNetworkReply*, void*user);
using DataProgress = void(*)(double now,double total,void*user);


class QAQNetworkInterface
{
public:
	virtual void OnDataReply(QAQNetworkReply*) = 0;

	virtual void OnDataProgress(double total, double now) {};

};

class QAQNetwork
{
public:
	QAQNetwork() {};
	virtual ~QAQNetwork() {};

	virtual QAQNetworkReply* SyncGet(QAQNetworkReq * req, void *user = nullptr, DataCallBack data_cb= nullptr, DataProgress pro_cb= nullptr) = 0;

	virtual QAQNetworkReply* SyncPost(QAQNetworkReq * req, const char*post_data, void *user = nullptr, DataCallBack data_cb = nullptr, DataProgress pro_cb = nullptr) = 0;

	virtual void ASyncGet(QAQNetworkReq *,void *user, DataCallBack, DataProgress = nullptr) = 0;

	virtual void ASyncPost(QAQNetworkReq *,const char*post_data, void *user, DataCallBack , DataProgress = nullptr)=0;

	virtual void ASyncGet2(QAQNetworkReq *, QAQNetworkInterface *) = 0;

	virtual void ASyncPost2(QAQNetworkReq *, const char*post_data,QAQNetworkInterface *)=0;

	virtual void Stop() = 0;

	virtual void CleanInterface() = 0;

	virtual const char * GetMemoryData()  = 0;

	virtual int GetMemorySize() = 0;
protected:

	QAQNetworkReply* reply_;
	QAQNetworkReq* req_;
};




class QAQNetworkReq
{
public:
	QAQNetworkReq() {};

	virtual ~QAQNetworkReq() {};

	virtual void SetHeader(const char * header) = 0;

	virtual void SetUrl(const char * url) = 0;

	virtual const char * GetHeader(int ) = 0;

	virtual int GetHeaderCount() = 0;

	virtual char * GetUrl() const = 0;

};


enum QAQNetworkReplyError
{
	REPLY_INIT,
	REPLY_NO_ERROR,
};


class QAQNetworkReply
{
public:
	QAQNetworkReply()
		: len_(-1)
		, error_(REPLY_INIT)
		, data_(nullptr)
	{}

	virtual ~QAQNetworkReply() {}


	virtual char* GetData()
	{
		return data_;
	}

	virtual int GetLength()
	{
		return len_;
	}
	virtual QAQNetworkReplyError GetError()
	{
		return error_;
	}

	virtual void SetData(char *data) {
		data_ = data;
	}

	virtual void SetLength(int len)
	{
		len_ = len;
	}

	virtual void SetError(QAQNetworkReplyError error)
	{
		error_ = error;
	}

private:
	QAQNetworkReplyError error_;
	char *data_;
	int len_;
};


extern "C" QAQNETWORK_LIB void CreateNetwork(QAQNetworkReq**,QAQNetwork**);

//QAQNETWORK_LIB QAQNetworkReq* CreateRequest();
//
////this is for sync
//QAQNETWORK_LIB void DeleteRequest(QAQNetworkReq*);
//QAQNETWORK_LIB void DeleteNetwork(QAQNetwork*);

//this is for async
extern "C" QAQNETWORK_LIB void DeleteNetwork(QAQNetworkReq**, QAQNetwork**);