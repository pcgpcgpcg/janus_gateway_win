#pragma once
#include <string>
class JanusHandle
{
public:
	JanusHandle();
	~JanusHandle();

private:
	 int handleId;
	 int feedId;
	 std::string display;
};

