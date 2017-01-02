#include "stdafx.h"
#include "Token_Manager.h"


Token_Manager::Token_Manager(const std::vector<std::vector<int>>& collection_group_items)
{
	//--1,N, -2, -3,
	/*
	starting at beginning of execution order array, execute evaluate on each symbol array.

	-1: if: go look up the X and Y from the buffer
	match that in the RBG table.
	if it equals (or doesn't equal if not), eval to true.

	-2: NOT: invert the result of this operations true/false evaluation

	-3: OR : if this or the next evaluates to false, continue. abort only if both false.

	-4: ifget:
	acces numerical vector.
	access condition array of numerical vector
	check if index equal to value
	if so, true.

	-5: do
	if each condition leading here is true enqueue action script into the queue.
	if false, skip to the next non- -5 or -6.

	-6: set
	if each condition leading to here is true, set this value at index to condition
	if false, skip to the next non- -5 or -6.
	*/
	std::regex regex_if("IF\\[\\d+.\\d+\\]"); //-1
	std::regex regex_name("[PRO](\\d+)?\\((\\d+,\\d+)(,\\d+,\\d+)?\\)"); //N
	std::regex regex_not("NOT"); //-2
	std::regex regex_or("OR"); //-3
	std::regex regex_set("SET_CONDITION\\[\\d+,\\d+\\]"); //-6
	std::regex regex_get("IF\\[GET_CONDITION\\[[PRO](\\d+)?,\\d+,\\d+\\]\\]"); //-4
	std::regex regex_do("DO\\[\\d+\\]"); //-5

										 //load vector with all regex tokens. 
	regex_tokens.push_back(regex_if);
	regex_tokens.push_back(regex_name);
	regex_tokens.push_back(regex_not);
	regex_tokens.push_back(regex_or);
	regex_tokens.push_back(regex_set);
	regex_tokens.push_back(regex_get);
	regex_tokens.push_back(regex_do);


	images_by_group = collection_group_items;
}

void Token_Manager::add_token(std::string & ident_dat)
{
	//inspect the token first character, if P or R build normally.  if O, need special construction. 
	//overwatch needs to be treated like a normal token, but is composed of many subregions and tokens itself
	//it is essencially a token manager in itself. 
	//provide an overwatch manager which intercepts all O strings
	//before this class exits, add a pointer ot the manager wherever Hmap reads o. 

	//if starts with o, redirect to overwatch builder. 
	if (ident_dat[0] == 'O') {
		//need to create a region of points over this area. 
		overwatch_manager.build_region(regex_tokens, ident_dat, images_by_group, name_ordering);
		//now how do we add this collection into the build order?
		//every time a get calls a foreign point, its entered with that ID. 
		//just check table. if O exists, do nothing. if not, add as last. 
		if (name_ordering.find("O") == name_ordering.end()) {
			//overwatch not encountered in a get call yet. add it. 
			name_ordering["O"] = name_ordering.size();
			tokens_build_order.push_back(overwatch_manager.deque_front());
			first_deque = true;
		}
		else {
			//o already exists, was called on a previous 
			if (first_deque == false) {
				tokens_build_order.push_back(overwatch_manager.deque_front());
				first_deque = true;
			}
		}
	}
	else {
		//dealing with a single object watchpoint or wregion. 
		Token* new_token = new Token(regex_tokens, ident_dat, images_by_group, name_ordering);
		tokens_build_order.push_back(new_token);
	}
}

Token const* Token_Manager::get_token(int fetch) const
{
	if (fetch < 0 || fetch >(tokens_build_order.size() - 1)) {
		return nullptr;
	}
	else {
		return tokens_build_order[fetch];
	}
}

std::string Token_Manager::toString() const
{
	std::string data;
	for (int i = 0; i < tokens_build_order.size(); ++i) {
		data += tokens_build_order[i]->toString() + '\n';
	}
	data += overwatch_manager.toString() + '\n';
	return data;
}

std::string Token_Manager::overwatch_conditions_toString() const
{
	return overwatch_manager.conditions_toString();
}

std::string Token_Manager::dbgPrintBuildHash() const
{
	std::string data;
	for (std::unordered_map<std::string, int>::const_iterator it = name_ordering.begin(); it != name_ordering.end(); ++it) {
		//v.push_back(it->first);
		data += (it->first) + " " + std::to_string((it->second)) + "\n";
	}
	return data;
}

void Token_Manager::build_execution_order(std::string& exe_order)
{
	std::vector<Token*> numerical_order;
	numerical_order.resize(name_ordering.size());
	std::vector<Token*> execution_order;
	execution_order.resize(name_ordering.size());

	//find 0, 1, 2, 3 ect in the map, and insert in the vector place, the copy vector 
	//how to handle overwatch? 
	//normally evaluate a symbol sequence then move on to the next symbol. 
	//check if each synbol as a next, none will but overwatch deque. 
	//do symbol evaluation on every point in overwatch, breaking when next is a nullptr. 

	for (int i = 0; i < tokens_build_order.size(); ++i) {
		//need two different collections. a list of by-number pointers and a list of by execution-order pointerss
		//build numerical order by going through 
		//look up name of each, find in map, add that pointer to numerical at the map value.
		std::string nm = tokens_build_order[i]->getName(); //get the name from the current tokens. 
		//-------crash here. 
		int numeric_addr = name_ordering.find(nm)->second; //get the identifer associated with that name. 
		numerical_order[numeric_addr] = tokens_build_order[i]; //that numerical identifier gets O(1) lookup to pointer at this array. 
	}


	//execution order: order pointers 
	std::string substr;
	int octr = 0;
	for (int i = 0; i < exe_order.length(); ++i) {
		if (exe_order[i] == ' ' || exe_order[i] == ';') {
			for (int r = 0; r < tokens_build_order.size(); ++r) {
				std::string nm = tokens_build_order[r]->getName(); //get the name from the current tokens.
				if (nm == substr) {
					//if names match, add the pointer to the execution order array. 
					execution_order[octr] = tokens_build_order[r];
					++octr;
				}
			}
			substr.clear();
		}
		else {
			substr += exe_order[i];
		}
	}

	tokens_identifier_order = numerical_order;
	for (int k = 0; k < tokens_identifier_order.size(); ++k) {
		std::cout << (int)numerical_order[k] << " ";
	}
	std::cout << std::endl;
	tokens_execution_order = execution_order;
	for (int k = 0; k < tokens_execution_order.size(); ++k) {
		std::cout << (int)execution_order[k] << " ";
	}

	//compact the overwatch deque into a vector. 
	overwatch_manager.pkg_deque_to_vector();
}

int Token_Manager::token_count() const
{
	return tokens_build_order.size();
}

int Token_Manager::getOverwatchNumeric() const
{
	//publish the numeric ID of overwatch so it can be intercepted in GET Overwatch condition calls. 
	int overwatchID = -99;
	for (std::unordered_map<std::string, int>::const_iterator itr = name_ordering.begin(); itr != name_ordering.end(); ++itr) {
		if (itr->first == "O") {
			overwatchID = itr->second;
		}
	}
	return overwatchID;
}

void Token_Manager::reset_all_condition_flags()
{
	//for every token in exe order, reset all internal flags to 0. 
	for (int i = 0; i < tokens_execution_order.size(); ++i) {
		for (int j = 0; j < tokens_execution_order[i]->get_condition_size(); ++j) {
			tokens_execution_order[i]->set_condition_value(j, 0); 
		}
	}
}

const std::vector<Token*>& Token_Manager::access_execution_order() const
{
	return tokens_execution_order;
}

const std::vector<Token*>& Token_Manager::access_numerical_order() const
{
	return tokens_identifier_order;
}

const std::vector<Token*>& Token_Manager::get_overwatch_points() const
{
	return overwatch_manager.get_overwatch_vector();
}

const int Token_Manager::get_overwatch_condition(int index) const
{
	return overwatch_manager.get_condition_value(index);
}

void Token_Manager::set_overwatch_value(int index, int value)
{
	overwatch_manager.set_condition_value(index, value);
}


Token_Manager::~Token_Manager()
{
}
