/*
This class is responsible for holding all the token objecte generated via watchobject+script synthesis.
It provides different vector style mappings of tokens. 
One is the order tokens were built in (assigned ID's in this order) which is then used to build the hashmap of all loaded tokens. 
From this hashmap, a vector of pointers to tokens is built in the order they will need to be executed to match the execution order correctness from the stencilpack
(Point 0 cannot GET point 1's location if point 1 is called before 0. It won't ever have the most recent data.) 

Overwatch is a very special singleton case of a watchpoint. While it may be accessed as a watchpoint, it really represents a region containing a scatter pattern. 
When building the overwatch, it is really generating a number of program-generated watchpoints to fill the region every Nth pixel instead. 
*/

#pragma once
#ifndef H_TOKENMGR
#define H_TOKENMGR

#include <regex>
#include <unordered_map>
#include "token.h"
#include "Overwatch_Builder.h"
#include <memory>
#include <iterator>
#include <iostream>

class Token_Manager
{
public:
	Token_Manager(const std::vector<std::vector<int>>&);
	void add_token(std::string &);
	Token const* get_token(int) const;
	std::string toString() const;
	std::string overwatch_conditions_toString() const;
	std::string dbgPrintBuildHash() const;
	void build_execution_order(std::string&);
	int token_count() const;
	int getOverwatchNumeric() const;
	const std::vector<Token*>& access_execution_order() const;
	const std::vector<Token*>& access_numerical_order() const;
	const std::vector<Token*>& get_overwatch_points() const;
	const int get_overwatch_condition(int) const;
	void set_overwatch_value(int, int);
	~Token_Manager();
private:
	std::vector<std::regex> regex_tokens;
	std::vector<std::vector<int>>  images_by_group;
	bool first_deque = false;
	std::unordered_map<std::string, int> name_ordering;

	Overwatch_Builder overwatch_manager; //the manager of the overwatch regions. Access like a normal point. 
	std::vector<Token*> tokens_build_order; //order tokens were built in. 

	std::vector<Token*> tokens_identifier_order; //use hmap so when all tokens are build, this refers to their hmap position. 
	std::vector<Token*> tokens_execution_order; //tokens update functions executed in this order. 
};

#endif