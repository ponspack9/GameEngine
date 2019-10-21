#pragma once
#include "Module.h"
#include "Globals.h"
#include "Mesh.h"

#define MAX_SNAKE 2

class ModuleSceneIntro : public Module
{
public:
	ModuleSceneIntro(bool start_enabled = true);
	~ModuleSceneIntro();

	bool Start(Config* config = nullptr);
	bool Update(float dt);
	bool Draw();
	bool PostUpdate(float dt);
	bool CleanUp();

	//Temporary mesh data
	Mesh* m;
};

void DrawGridPlane();
