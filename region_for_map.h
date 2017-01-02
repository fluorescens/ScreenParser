#pragma once
/*
Used to build a map of region identifiers and their respecitve coordinates. 
Used to watchobjects can call for a file slice to different identifiers and quickly extract their coordinates. 
*/
#include <string>

struct region_for_map
{
	region_for_map(int inx, int iny, int inlength, int inheight, std::string inindent) : 
		x(inx), y(iny), length(inlength), height(inheight), identifier(inindent){

	}
	int x; 
	int y;
	int length; 
	int height; 
	std::string identifier; 
};

