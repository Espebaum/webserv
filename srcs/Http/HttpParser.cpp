#include "HttpParser.hpp"

HttpRequest	HttpParser::parse(Client& client)
{
	HttpRequest		result;
	RequestMessage	request;
	
	request = client.getReq();

	if (request.requestTarget.find("cgi-bin") == std::string::npos)
	{
		// When static
		result.is_static = true;
		result.path = request.requestTarget;
		std::cout << BOLDYELLOW << "URI : " << request.requestTarget << '\n';
		result.file_name = "./html" + result.path;
		std::cout << BOLDGREEN << "FILE NAME : " << result.file_name << '\n';
	} // now working here STATIC
	else
	{
		result.is_static = false;
		size_t	location = request.requestTarget.find('?');
		if (location != std::string::npos)
			result.cgi_args = request.requestTarget.substr(location + 1);
		result.file_name = "." + request.requestTarget.substr(0, location);
		result.body = request.body;
	}
	return result;
};
