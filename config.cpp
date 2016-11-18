#include "config.h" 


std::string GetHashCommand ()
{
		//FIX
		return "md5sum";
}

std::map <std::string, std::string> GetIndexMask ()
{
		//FIX
		std::map<std::string, std::string> Mask;
		Mask["odt"] = "odt2txt";
		Mask["doc"] = "catdoc";
		Mask["txt"] = "cat";
		return Mask;
}


std::string GetHomeFolder ()
{
	//FIX
	return "/home/f/merc/docfinder";
}
