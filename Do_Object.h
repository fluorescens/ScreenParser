/*
These represent the action script string arguments which will be forwarded (possibly to an interprogram cache queue) once their watchobject trigger is true. 
Each of these has a priority assigned based on their first argument from the stencilpack so no matter which order the watchregions add them in,
they are popped from a priority queue in the largest-priority-first order. 

Their string arguments should be pipelined to a seperate shell which can execute keystrokes or windows events based on their own custom implementation,
allowing for incredible user flexibility. 
*/

#pragma once
#include <string>
class Do_Object
{
public:
	Do_Object(int, std::string);
	Do_Object(const Do_Object&);
	std::string getDoStr() const;
	int getPriority() const;
	void setx(int);
	void sety(int);
	int getx() const;
	int gety() const;
	~Do_Object();
private:
	int priority;
	int callerx;
	int callery;
	std::string cmd;
};

