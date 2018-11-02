#pragma once
#include <string>
#include <functional>

class JanusTransaction
{
public:
	JanusTransaction();
	~JanusTransaction();

public:
	std::string transactionId;
	std::function<void(int,std::string)> Success;
	std::function<void(std::string, std::string)> Error;//error code and error desc
	std::function<void(std::string)> Event;//event with json message as param
};

