#pragma once
#include "Component.h"
#include <string>

#include "glew\include\GL\glew.h"

class ComponentMaterial : public Component
{
public:

	ComponentMaterial(GameObject* obj);
	virtual ~ComponentMaterial();

public:
	uint  tex_id = 0;
	const char* path;
	uint width = 0;
	uint height = 0;
};
