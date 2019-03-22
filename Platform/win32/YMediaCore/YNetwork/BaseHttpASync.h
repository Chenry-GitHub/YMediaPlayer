#pragma once
#include <QAQNetwork.h>
#include <string>
class BaseHttpAsync:public QAQNetworkInterface
{
public:
	BaseHttpAsync()
	{
		CreateNetwork(&req_, &network_);

	}
	virtual ~BaseHttpAsync()
	{
		DeleteNetwork(&req_, &network_);
	}

	QAQNetwork * GetNetwork()
	{
		return network_;
	}

	QAQNetworkReq* GetNetworkRequest()
	{
		return req_;
	}

	virtual void OnDataReply(QAQNetworkReply*reply) override
	{
		/*throw std::logic_error("The method or operation is not implemented.");*/
		bool is_succ_ = false;
		if (reply->GetError() == REPLY_NO_ERROR)
		{
			if (OnAnalysis(std::string(reply->GetData() , reply->GetLength())))
			{
				is_succ_ = true;
			}
		}
		OnAfterHandleReply(is_succ_);
	}
	virtual bool OnAnalysis(const std::string &data) = 0;

	virtual void OnAfterHandleReply(bool) = 0;


	

private:
	QAQNetworkReq * req_;
	QAQNetwork * network_;
};