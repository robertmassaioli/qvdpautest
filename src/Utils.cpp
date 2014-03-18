#include "Utils.h"
#include "PciDisplay.h"

#include <algorithm>
#include <fstream>
#include <string>

template<class TContainer>
bool beginsWith(const TContainer& input, const TContainer& match)
{
	return input.size() >= match.size()
		&& equal(match.begin(), match.end(), input.begin());
}

std::string getCPUModel(void)
{
	std::ifstream ifs( "/proc/cpuinfo" );

	if ( ifs )
	{
		std::string line;

		while ( std::getline(ifs, line) )
		{
			if ( beginsWith(line, std::string("model name") ) )
			{
				std::size_t pos(line.find(":"));

				if ( pos != std::string::npos )
					return line.substr(pos + 2);
			}
		}
	}

	return "Unknown CPU";
}

std::string getGPUModel(void)
{
	PciDisplay pd;
	return pd.getFirstDisplay();
}

