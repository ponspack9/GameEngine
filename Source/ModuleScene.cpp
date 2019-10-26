#include "ModuleScene.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ComponentRenderer.h"
#include "ComponentMaterial.h"

ModuleScene::ModuleScene(bool start_enabled) : Module("Scene", start_enabled)
{
}


ModuleScene::~ModuleScene()
{
}

// Load assets
bool ModuleScene::Start(Config* config)
{
	LOG("Loading Intro assets", 'v');
	bool ret = true;

	App->camera->Move(vec3(0, 7.5f, 7.5f));
	App->camera->LookAt(vec3(0, 0, 0));
	
	GameObject* bparent = CreateGameObj("BakerHouse");

	App->resources->LoadResource("Assets/BakerHouse.fbx", Component::Type::Mesh, true,bparent->GetUID());
	App->resources->LoadResource("Assets/Baker_house.png", Component::Type::Material, true);

	GameObject* pparent = CreateGameObj("ParShapes");
	for (int i = 0; i < shape_type::UNKNOWN; i++)
	{
		App->resources->CreateShape((shape_type)i, 9, 9, i * 7.5 - 50, 2.5f, -10, 0.5f, pparent->GetUID());
	}

	App->editor->tab_hierarchy->UnSelectAll();

	return ret;
}

bool ModuleScene::Update(float dt)
{
	return true;
}

bool ModuleScene::PostUpdate(float dt)
{
	return true;
}

bool ModuleScene::CleanUp()
{
	LOG("Unloading Main scene");

	for (uint i = 0; i < gameobjs.size(); ++i)
	{
		RELEASE(gameobjs[i]);
	}
	gameobjs.clear();

	for (uint i = 0; i < materials.size(); ++i)
	{
		RELEASE(materials[i]);
	}
	materials.clear();

	return true;
}

bool ModuleScene::Draw()
{
	// Draw GameObjects
	for (uint i = 0; i < gameobjs.size(); ++i)
	{
		glPushMatrix();
		glMultMatrixf(gameobjs[i]->GetLocalTransform().ptr());

		ComponentRenderer* renderer = (ComponentRenderer*)gameobjs[i]->GetComponent(Component::Type::Renderer);
		if (renderer != nullptr && renderer->IsActive())
		{
			if (renderer->show_wireframe) //wireframe
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			renderer->Draw();
		}
		glPopMatrix();
	}
	return true;
}

GameObject* ModuleScene::CreateGameObj(const char* name, const uint parent_id, bool visible)
{
	create_gameobj = true;

	GameObject* obj = new GameObject(name);
	if (visible)
		obj->AddComponent(Component::Type::Renderer);

	gameobjs.push_back(obj);


	App->editor->tab_hierarchy->AddNode(obj, App->editor->tab_hierarchy->SearchById(parent_id)); // add node to hierarchy
	selected_gameobj = obj; // new obj is selected_gameobj

	return obj;
}

void ModuleScene::DeleteGameobj(GameObject* obj)
{
	if (selected_gameobj == obj)
		selected_gameobj = nullptr;

	for (uint i = 0; i < gameobjs.size(); i++)
	{
		if (gameobjs[i] == obj)
		{
			RELEASE( gameobjs[i]);
			gameobjs.erase(gameobjs.begin() + i);
			break;
		}
	}
}

bool ModuleScene::IsMaterialLoaded(const char* path)
{
	for (uint i = 0; i < materials.size(); i++)
	{
		if (strcmp(materials[i]->path, path) == 0)
			return true;
	}
	return false;
}

ComponentMaterial* ModuleScene::GetMaterial(const char* path) const
{
	for (uint i = 0; i < materials.size(); ++i)
	{
		if (strcmp(materials[i]->path, path) == 0)
			return materials[i];
	}
	return nullptr;
}

void ModuleScene::DeleteMaterial(ComponentMaterial* material)
{
	for (uint i = 0; i < materials.size(); i++)
	{
		if (materials[i] == material)
			materials.erase(materials.begin() + i);
	}
}

void ModuleScene::EraseFromSelected(GameObject * go)
{
	if (go == nullptr) return;

	for (uint i = 0; i < selected_go.size(); i++)
	{
		if (selected_go[i] != nullptr && selected_go[i]->GetUID() == go->GetUID())
		{
			selected_go.erase(selected_go.begin() + i);
			return;
		}
	}
}
