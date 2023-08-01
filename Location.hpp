#pragma once
#include <map>
#include <string>
#include <vector>

class Location
{
private:
    std::string root;
    std::map<std::vector<int>, std::string> errorPage;
    std::map<std::string, int> redirection;
    unsigned int allowMethod;
    std::vector<std::string> index;
    bool autoIndex;
    size_t clientBodySize;

    Location(Location const& other);
    Location& operator=(Location const& rhs);

    void setRoot(std::string& input);
    bool setErrorPage(std::string const& sentence);
    bool setRedirection(std::string const& sentence);
    void setAutoIndex(std::string& input);
    void setClientBodySize(std::string& input);
    bool setAllowMethod(std::string& sentence);
    void setIndex(std::string& sentence);
public:
    Location();
    ~Location();
    bool fillLocationBlock(std::map<std::string, std::string>& mapSentence);
    void DebugPrint();

};
