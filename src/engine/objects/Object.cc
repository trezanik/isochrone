
#if TZK_USING_RCCPP
#include "engine/objects/Object.h" // Base class for all objects


void Object::Update(float delta_time)
{
	//std::cout << "Runtime Object Update() called!\n";
}


REGISTERCLASS(Object);

#endif
