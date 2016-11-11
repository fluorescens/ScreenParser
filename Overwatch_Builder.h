/*
Overwatch is a very special region. While providing a pointer-to-token like interface for continuity, it really is a collection of watchpoint tokens.
An overwatch is generated as taking Token(script)(bounds) and generating a watchpoint every Nth pixel within the bounds with the same script.

In essence, overwatch is shorthand for "lightly watch this entire region with pixel_area/N pixels, and generate the watchpoints automatically."
*/

#pragma once

#ifndef H_OVERWATCH_BUILDER
#define H_OVERWATCH_BUILDER

#include "Token.h"
#include <iostream>
#include <memory>
#include <deque>
#include <vector>


class Overwatch_Builder
{
public:
	Overwatch_Builder();
	void build_region(const std::vector<std::regex>&,
		const std::string &,
		const std::vector<std::vector<int>>&,
		std::unordered_map<std::string, int>&);
	std::string toString() const;
	std::string conditions_toString() const;
	Token* deque_front() const;
	void pkg_deque_to_vector();
	std::vector<Token*>const & get_overwatch_vector() const;
	void set_condition_value(int, int);
	const int get_condition_value(int) const;
	~Overwatch_Builder();
private:
	std::deque<Token*> token_deque; //to preserve order, need a deque type. 
	std::vector<Token*> token_vector;
	Token* last_object = nullptr;
	std::vector<int> conditions;
};

#endif // !H_OVERWATCH_BUILDER