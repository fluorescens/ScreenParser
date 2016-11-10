#include "stdafx.h"
#include "token.h"



Token::Token(const std::vector<std::regex>& rgx,
	const std::string & source,
	const std::vector<std::vector<int>>& img_by_group,
	std::unordered_map<std::string, int>& hmap)
{
	std::string csubstr;
	int back = 0;
	int cmax_reserve = 0; //reserve the largest condition flag. 
	bool bind_not = false;
	bool bound_or = false;
	for (unsigned int i = 0; i < source.length(); ++i) {
		if (source[i] == ' ' || source[i] == ';') {
			//force it to equal a token, or error.
			if (std::regex_match(csubstr, rgx[0])) {
				//if item code  = -1 
				std::vector<int> ifdat;
				ifdat.push_back(-1); //lookup type
				int group = 0;
				int ident = 0;
				std::string local_sub;
				for (int k = 3; k < csubstr.length(); ++k) {
					if (csubstr[k] == '.') {
						group = std::stoi(local_sub);
						local_sub.clear();
					}
					else if (k == csubstr.length() - 1) {
						ident = std::stoi(local_sub);
						local_sub.clear();
						break;
					}
					else {
						local_sub += csubstr[k];
					}
				}
				int item_code = img_by_group[group][ident]; //retrieve item id 0
				if (bind_not == true) {
					//if set, logically invert the return
					ifdat.push_back(-2);
					bind_not = false;
				}
				ifdat.push_back(item_code); //evaluate this item
				symbols.push_back(ifdat); //add this vector to symbols. 
				csubstr.clear();
			}
			else if (std::regex_match(csubstr, rgx[1])) {
				//set up the name and dimension properties. 
				//if you don't find yourself in the map already. self-register. 
				std::string tmp;
				int comma_counter = 0;
				for (int k = 0; k < csubstr.length(); ++k) {
					if ((csubstr[k] == '(') || (csubstr[k] == ',') || (csubstr[k] == ')')) {
						if ((csubstr[k] == '(')) {
							name = tmp;
							if (hmap.find(tmp) == hmap.end()) {
								//no such key is in the map yet, need to add. 
								hmap[tmp] = hmap.size();
							}
							else {
								//key is contained in the map already. 
							}
						}
						else if (csubstr[k] == ',') {
							switch (comma_counter) {
							case 0:
								ul_x = std::stoi(tmp);
								break;
							case 1:
								ul_y = std::stoi(tmp);
								break;
							case 2:
								lr_x = std::stoi(tmp);
								break;
							}
							++comma_counter;
						}
						else if (csubstr[k] == ')') {
							if (comma_counter == 1) {
								ul_y = std::stoi(tmp);
							}
							else if (comma_counter == 3) {
								lr_y = std::stoi(tmp);
							}
						}
						tmp.clear();
					}
					else {
						tmp += csubstr[k];
					}
				}
				csubstr.clear();
			}
			else if (std::regex_match(csubstr, rgx[2])) {
				//bind to NEXT, -2
				bind_not = true;
				csubstr.clear();
			}
			else if (std::regex_match(csubstr, rgx[3])) {
				//bind to previous IF, -3
				if (symbols.size() == 0) {

				}
				else {
					int ldex = symbols.size() - 1;
					symbols[ldex].push_back(-3);
				}
				csubstr.clear();
			}
			else if (std::regex_match(csubstr, rgx[4])) {
				//need to up to the largest set value. set[0,1] set[5,2] set[400,3]  
				//set -6
				std::vector<int> subdata;
				subdata.push_back(-6);
				std::string subint;
				int extract_val = 0;
				for (int k = 14; k < csubstr.length(); ++k) {
					if (csubstr[k] == ',' || (k == csubstr.length() - 1)) {
						if (csubstr[k] == ',') {
							extract_val = std::stoi(subint);
							subdata.push_back(extract_val);
						}
						else if (k == csubstr.length() - 1) {
							int set_to = std::stoi(subint);
							subdata.push_back(set_to);
						}
						subint.clear();
					}
					else {
						subint += csubstr[k];
					}
				}
				if (extract_val > cmax_reserve) {
					cmax_reserve = extract_val;
				}
				//so we know how many to allocate...but how do we track what up update? 
				//push onto the boolean array a -6, index value, set value. 
				symbols.push_back(subdata);
				csubstr.clear();
			}
			else if (std::regex_match(csubstr, rgx[5])) {
				//foreign lookup -4, go to hmap, access the internal array, and query for the state of this index. 
				std::vector<int> get_vec;
				get_vec.push_back(-4);
				if (bind_not == true) {
					//if set, logically invert the return
					get_vec.push_back(-2);
					bind_not = false;
				}
				std::string tmp;
				int comma_counter = 0;
				for (int k = 17; k < csubstr.length(); ++k) {
					//how do we represent the key like P77 or R12 or O as an int? Keep a seperate vector of foreign key requests
					if (csubstr[k] == ',' || csubstr[k] == ']') {
						switch (comma_counter) {
						case 0:
							//first substring is the foreign object key
							if (hmap.find(tmp) == hmap.end()) {
								//no such key is in the map yet, need to add. 
								hmap[tmp] = hmap.size();
								get_vec.push_back(hmap.size() - 1);
							}
							else {
								//key is contained in the map already. 
								get_vec.push_back(hmap[tmp]); //push the map value (its added order) onto the local vec
							}
							break;
						case 1:
							//second object is the condition that is requested. 
							get_vec.push_back(std::stoi(tmp));
							break;
						case 2:
							//third is the value to test against. 
							get_vec.push_back(std::stoi(tmp));
							break;
						}
						tmp.clear();
						++comma_counter;
					}
					else {
						tmp += csubstr[k];
					}
				}
				symbols.push_back(get_vec);
				csubstr.clear();
			}
			else if (std::regex_match(csubstr, rgx[6])) {
				//identifier -5, do
				std::vector<int> local_data;
				//extract identifier. 
				std::string substr;
				for (int k = 3; k < csubstr.length(); ++k) {
					if (k == csubstr.length() - 1) {
						break;
					}
					else {
						substr += csubstr[k];
					}
				}
				local_data.push_back(-5);
				local_data.push_back(std::stoi(substr));
				symbols.push_back(local_data);
				csubstr.clear();
			}
			else {
				//substring matched no regex 
			}
		}
		else {
			csubstr += source[i];
		}
	}

	int flags_size = cmax_reserve + 1; //assure the largest set flag can be indexed
									   //try and alloc the flags
	try {
		condition_flags.resize(flags_size);
	}
	catch (std::bad_alloc& e) {
		//No available memory, cannot build tables. 
		std::abort();
	}
}

Token::Token(const Token const* target)
{
	symbols = target->symbols;
	name = target->name;  //map identifier and token string name
	ul_x = target->ul_x; //coords for the point
	ul_y = target->ul_y;
	lr_x = target->lr_x;
	lr_y = target->lr_y;
	try {
		condition_flags = target->condition_flags;
	}
	catch (std::bad_alloc &e) {
		std::abort();
	}
}

void Token::modx(int a)
{
	ul_x = a;
}

void Token::mody(int a)
{
	ul_y = a;
}

Token & Token::operator=(const Token & target)
{
	//deep-copy all the values and return a pointer to the new object. 
	symbols = target.symbols;
	name = target.name;  //map identifier and token string name
	ul_x = target.ul_x; //coords for the point
	ul_y = target.ul_y;
	lr_x = target.lr_x;
	lr_y = target.lr_y;
	try {
		condition_flags = target.condition_flags;
	}
	catch (std::bad_alloc &e) {
		std::abort();
	}
	return *this;
}



std::string Token::toString() const
{
	std::string as_string;
	as_string += name + " " + std::to_string(ul_x) + " " + std::to_string(ul_y) + " " + std::to_string(lr_x) + " " + std::to_string(lr_y) + '\n';
	for (int i = 0; i < symbols.size(); ++i) {
		for (int j = 0; j < symbols[i].size(); ++j) {
			as_string += std::to_string(symbols[i][j]) + " ";
		}
		as_string += '\n';
	}
	return as_string;
}

std::string Token::toStringConditions() const
{
	std::string condition_values;
	for (int i = 0; i < condition_flags.size(); ++i) {
		condition_values += std::to_string(condition_flags[i]) + " ";
	}
	return condition_values;
}

void Token::setNext(Token* tk)
{
	nextobj = tk;
}

Token * Token::getNext()
{
	return this->nextobj;
}

std::string Token::getName() const
{
	return name;
}

const std::vector<std::vector<int>>& Token::access_symbols() const
{
	return symbols;
}

int Token::get_ulx() const
{
	return ul_x;
}

int Token::get_uly() const
{
	return ul_y;
}

int Token::get_lrx() const
{
	return lr_x;
}

int Token::get_lry() const
{
	return lr_y;
}

void Token::set_condition_value(int index, int value)
{
	condition_flags[index] = value;
}

int Token::get_condition_value(int index) const
{
	return condition_flags[index];
}

int Token::get_condition_size() const
{
	return condition_flags.size();
}

Token::~Token()
{
}
