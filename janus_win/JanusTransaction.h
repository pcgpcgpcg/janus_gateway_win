#pragma once
#include <string>

class TransactionEvents {
	virtual void success(int id) = 0;
	virtual void success(int id, std::string jsep) = 0;
	virtual void error(std::string reason, std::string code) = 0;
};

class JanusTransaction
{


public:
	JanusTransaction();
	~JanusTransaction();

public:
	std::string transactionId;
	//TransactionEvents events;
};

