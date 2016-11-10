#include "stdafx.h"
#include "Do_Object.h"


Do_Object::Do_Object(int inpriority, std::string instr)
{
	priority = inpriority;
	cmd = instr;
}

Do_Object::Do_Object(const Do_Object &)
{
}

std::string Do_Object::getDoStr() const
{
	std::string datout = std::to_string(callerx) + " " + std::to_string(callery) + " " + cmd;
	return datout;
}

int Do_Object::getPriority() const
{
	return priority;
}

void Do_Object::setx(int setxto)
{
	callerx = setxto;
}

void Do_Object::sety(int setyto)
{
	callery = setyto;
}

int Do_Object::getx() const
{
	return callerx;
}

int Do_Object::gety() const
{
	return callery;
}


Do_Object::~Do_Object()
{
}
