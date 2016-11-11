

#include "stdafx.h"
#include "Overwatch_Builder.h"



Overwatch_Builder::Overwatch_Builder()
{

}

void Overwatch_Builder::build_region(const std::vector<std::regex>& rgx,
	const std::string & source,
	const std::vector<std::vector<int>>& img_by_group,
	std::unordered_map<std::string, int>& hmap)
{
	//build ONE token object at the rectangle ul point with the full script. 
	//then alloc a vector for the total number of points/10 
	//then in each spot, copy the token in index 0 except we modify the coordinates. 
	//THEN we all this massive vector onto end of the builder object vector in manager so all of the points are tested. 
	//name doesn't matter for this, only coordinates

	//O(x,y,w,h)

	//step 1: extract dimensions from source. 
	int x0 = 0;
	int y0 = 0;
	int w = 0;
	int h = 0;

	int comma_count = 0;
	std::string substr;
	for (int r = 0; r < source.length(); ++r) {
		//set on ( , , , )
		if (source[r] == '(') {
			substr.clear();
		}
		else if (source[r] == ',') {
			switch (comma_count) {
			case 0:
				x0 = std::stoi(substr);
				break;
			case 1:
				y0 = std::stoi(substr);
				break;
			case 2:
				w = std::stoi(substr);
				break;
			}
			++comma_count;
			substr.clear();
		}
		else if (source[r] == ')') {
			h = std::stoi(substr);
			break;
		}
		else {
			substr += source[r];
		}
	}

	const int SCALE_FACTOR = 2;
	int create_pts = ((w / SCALE_FACTOR) + 1)  * ((h / SCALE_FACTOR) + 1); //SCALE FACTOR. Creates a new token every 50 pixels. Increase or reduce for load. 
																		   //this creates one every 10 on the line. want a Y-dispersal as well. so skip 10 on x AND y. So at end of xline, jump down 10 y. 

	Token* init_token = new Token(rgx, source, img_by_group, hmap); //needs to have last deque point to it, and update to current
	if (last_object == nullptr) {
		token_deque.emplace_back(init_token); //first token ever added. 
	}
	else {
		Token* back_token = token_deque.back();
		back_token->setNext(init_token); //update last token in deque so its next points 
		token_deque.emplace_back(init_token); //first token ever added. 
	}


	int currentx = x0 + SCALE_FACTOR;
	int currenty = y0;
	for (int k = 1; k < create_pts; ++k) {
		if (currentx >(x0 + w)) {
			currentx = x0;
			currenty = currenty + SCALE_FACTOR;
		}
		if (currentx > (x0 + w) && currenty > (y0 + h)) {
			//failsafe to make sure no out of bounds are added. 
		}
		else {
			Token* tmp_token = new Token(init_token); //copy all arrays and symbols, change coordinates. 
			tmp_token->modx(currentx);
			tmp_token->mody(currenty);

			Token* back_token = token_deque.back();
			back_token->setNext(tmp_token); //update last token in deque so its next points 
			token_deque.emplace_back(tmp_token);

			//token_source.push_back(tmp_token); 
			currentx = currentx + SCALE_FACTOR;
		}
	}
	conditions.resize(init_token->get_condition_size());
}

std::string Overwatch_Builder::toString() const
{
	std::string token_data;
	/*
	for (int i = 0; i < token_source.size(); ++i) {
	token_data += token_source[i]->toString();
	}
	*/
	for (int i = 0; i < token_deque.size(); ++i) {
		token_data += token_deque.at(i)->toString();
	}

	return token_data;
}

std::string Overwatch_Builder::conditions_toString() const
{
	std::string tmpcondit;
	for (int i = 0; i < conditions.size(); ++i) {
		tmpcondit += conditions[i] + " ";
	}
	return tmpcondit;
}

Token * Overwatch_Builder::deque_front() const
{
	std::cout << (int)token_deque.front() << '\n';
	return token_deque.front();
}

void Overwatch_Builder::pkg_deque_to_vector()
{
	//packages the deque into a vector for use in recursive calls from symbolic parser. 
	for (std::deque<Token*>::const_iterator kit = token_deque.begin(); kit != token_deque.end(); ++kit) {
		token_vector.push_back((*kit));
	}
}

std::vector<Token*> const& Overwatch_Builder::get_overwatch_vector() const
{
	//returns the vector of overwatch-created points for use in symbolic parser recursive calls. 
	return token_vector;
}

void Overwatch_Builder::set_condition_value(int index, int value)
{
	conditions[index] = value;
}

const int Overwatch_Builder::get_condition_value(int index) const
{
	return conditions[index];
}


Overwatch_Builder::~Overwatch_Builder()
{
}
