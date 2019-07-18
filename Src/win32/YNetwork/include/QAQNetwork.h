#pragma once
/*
@charset:UTF-8
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
class QAQNetwork
{
public:
	class Delegate
	{
	public:
		/*return true ,the reply will not cache the data anymore*/
		virtual bool onTrunk(QAQNetwork *, char *data, int len) { return false; };
		/*finally reply*/
		virtual void onReply(QAQNetwork *,QAQNetworkReply*) {};
		/*progressing*/
		virtual void onDataProgress(QAQNetwork *,double total, double now) {};
	};

	virtual QAQNetworkReq* getRequest() = 0;

	virtual QAQNetworkReply* getReply() = 0;

	virtual void syncGet() = 0;

	virtual void syncPost() = 0;

	virtual void asyncGet() = 0;

	virtual void asyncPost()=0;

	virtual void stopAsync() = 0;

	virtual void setUserData(void *opaque) = 0;

	virtual void* getUserData() = 0;

	virtual void setDelegate(QAQNetwork::Delegate*dele) = 0;
};




class QAQNetworkReq
{
public:
	virtual void setHeader(const char * header) = 0;

	virtual void setUrl(const char * url) = 0;

	virtual const char * getHeader(int ) = 0;

	virtual int getHeaderCount() = 0;

	virtual const char * getUrl() const = 0;

	virtual void setPostData(const char *) = 0;
};


enum QAQNetworkReplyError
{
	REPLY_INIT,
	REPLY_NO_ERROR,
	REPLY_TIME_OUT,
};


class QAQNetworkReply
{
public:
	virtual const char* getData() = 0;

	virtual unsigned int getLength() = 0;

	virtual QAQNetworkReplyError getError() = 0;

};

/*Its not thread-safe API ,They are pair to be invoked in the same thread*/

extern "C" QAQNETWORK_LIB void CreateNetwork(QAQNetwork**, QAQNetwork::Delegate *);

extern "C" QAQNETWORK_LIB void DeleteNetwork(QAQNetwork**);