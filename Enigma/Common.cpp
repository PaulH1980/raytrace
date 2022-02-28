#include "Common.h"

namespace RayTrace
{

    std::string ToLower(std::string _in)
    {
        std::transform(_in.begin(), _in.end(), _in.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return _in;
    }

    std::string ToUpper(std::string _in)
    {
        std::transform(_in.begin(), _in.end(), _in.begin(),
            [](unsigned char c) { return std::toupper(c); });
        return _in;
    }

    bool EndsWith(const std::string& _input, const std::string& _ending)
    {
        if (_ending.empty()) return false;
        if (_input.length() >= _ending.length()) {
            return (0 == _input.compare(_input.length() - _ending.length(), _ending.length(), _ending));
        }
        else {
            return false;
        }
    }

}

