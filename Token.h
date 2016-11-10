/*
Each watchobject and its associated trigger script are composed into a single token.
This token can be loaded into the main execution parser and will contain all the information needed to relate it to other tokens and provide lookup information. 

*/

#pragma once
#ifndef H_TOKEN
#define H_TOKEN

#include <vector>
#include <regex>
#include <unordered_map>
#include <string>
#include <memory>

#include <iostream>

class Token
{
public:
	Token(const std::vector<std::regex>&, const std::string&, const std::vector<std::vector<int>>&, std::unordered_map<std::string, int>&); //source string, item and action arrays
	Token(const Token const*);
	void modx(int); //manually update 
	void mody(int);
	Token& Token::operator=(const Token&); //used to construct the overwatch dummy points. 
	std::string toString() const;
	std::string toStringConditions() const;
	void setNext(Token*);
	Token* getNext();
	std::string getName() const;
	const std::vector<std::vector<int>>& access_symbols() const; //provides access to the individual tokens symbolic vector. 

	//gets the bounds of the coordinates that represent where this region sits in the original stencil
	/*
	Default to -1. Watchpoints only have a ulx and uly. 
	*/
	int get_ulx() const;
	int get_uly() const;
	int get_lrx() const;
	int get_lry() const;

	void set_condition_value(int, int);
	int get_condition_value(int) const;

	int get_condition_size() const;

	~Token();
private:
	std::string name;  //map identifier and token string name
	int ul_x = -1; //coords for the point
	int ul_y = -1;
	int lr_x = -1;
	int lr_y = -1;
	std::vector<int> condition_flags;
	std::vector<std::vector<int>> symbols;
	Token* nextobj = nullptr; //used to accomodate deque-type building 
};

#endif TOKEN