#include "ModuleScene.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ComponentRenderer.h"
#include "ComponentMaterial.h"
#include "ComponentCamera.h"
#include "ComponentMesh.h"
#include "ModuleSceneBase.h"
#include "ResourceModel.h"
#include "Viewport.h"
#include "Quadtree.h"


#include "mmgr/mmgr.h"

GameObject* ModuleScene::root_object;
SceneState ModuleScene::state = EDIT;
const char* ModuleScene::state_to_string[STOP+1] = { "EDIT","START","PLAY","PAUSE","STOP" };

ModuleScene::ModuleScene(bool start_enabled) : Module("Scene", start_enabled)
{}


ModuleScene::~ModuleScene()
{
	delete root_object;
}

bool ModuleScene::Init(Config* config)
{
	// Does not show up in Hierarchy because it's created before the root node is created, so it's the only true free GameObject
	viewport_camera = (ComponentCamera*)App->scene->CreateGameObject("Viewport Camera")->AddComponent(Component::Type::Camera);

	viewport_camera->Move({ 0, 0, 25.0f });
	viewport_camera->Look({ 0, 0, 0 });
	viewport_camera->SetFarPlane(500.0f);

	root_object = new GameObject("Root", nullptr);
	root_object->uid = 0;
	// Create game objects after this ^^^^^^^^^^^^	

	return true;
}

// Load assets
bool ModuleScene::Start(Config* config)
{
	LOG("Loading main scene", 'v');
	
	scene_name = "Default_Scene";

	test_camera_obj = CreateGameObject("Main camera");
	test_camera_obj->AddComponent(Component::Type::Camera);

	test_camera = test_camera_obj->GetComponent<ComponentCamera>();

	//App->resources->ImportFromPath("/Assets/Street/Street environment_V01.FBX");
	
	quadtree = new Quadtree(AABB(Quadtree::min_point,Quadtree::max_point));

	RedoQuatree();

	return true;
}

bool ModuleScene::Update(float dt)
{
	if (test_camera)
		test_camera->DrawFrustum();

	if (quadtree->debug)
		quadtree->Draw();

	return true;
}

void ModuleScene::UpdateTransformationGuizmos()
{
	

	ImGuizmo::MODE mode_to_apply = curr_guizmo_mode;

	if (current_guizmo_operation == ImGuizmo::OPERATION::SCALE)
		mode_to_apply = ImGuizmo::MODE::WORLD;

	for (std::vector<GameObject*>::iterator it = gameObjects.begin(); it != gameObjects.end(); ++it)
	{
		if (!(*it)->is_selected) continue;

		float4x4 global_transform_trans = (*it)->GetGlobalTransform().Transposed();

		

		float t[16];

		ImGuizmo::Manipulate(viewport_camera->view_matrix4x4.Transposed().ptr(),
			viewport_camera->projection_matrix4x4.Transposed().ptr(),
			ImGuizmo::OPERATION::TRANSLATE,
			ImGuizmo::MODE::WORLD,
			global_transform_trans.ptr(), t);

		float4x4 moved_transformation = float4x4(
			t[0], t[4], t[8], t[12],
			t[1], t[5], t[9], t[13],
			t[2], t[6], t[10], t[14],
			t[3], t[7], t[11], t[15]);

		if (ImGuizmo::IsUsing())
		{
			switch (current_guizmo_operation)
			{
			case ImGuizmo::OPERATION::TRANSLATE:
			{
				float4x4 new_trans = moved_transformation * (*it)->GetGlobalTransform();
				(*it)->SetTransform(new_trans);
			}
			break;

			case ImGuizmo::OPERATION::ROTATE:
			{
				float4x4 new_trans = moved_transformation * (*it)->GetGlobalTransform();
				(*it)->SetTransform(new_trans);
			}
			break;
			case ImGuizmo::OPERATION::SCALE:
			{
				float4x4 save_trans = moved_transformation;
				moved_transformation = moved_transformation * last_moved_transformation.Inverted();

				float4x4 new_trans = moved_transformation * (*it)->GetGlobalTransform();
				(*it)->SetTransform(new_trans);

				last_moved_transformation = save_trans;
			}
			break;
			}
		}
		else
		{
			last_moved_transformation = float4x4::identity;
		}
	}

}
#include "Canvas.h"

bool ModuleScene::PostUpdate(float dt)
{
	// Do all remaining actions activated by flags
	for (uint i = 0; i < gameObjects.size(); i++)
	{
		GameObject* obj = gameObjects[i];

		if (obj->flags & ProcessNewChild)
		{
			obj->ChildAdded();
			obj->flags &= ~ProcessNewChild;
		}
		if (obj->flags & ProcessDeletedChild)
		{
			obj->ChildDeleted();
			obj->flags &= ~ProcessDeletedChild;
		}
		if (obj->flags & ProcessTransformUpdate)
		{
			obj->UpdateTransform();
			obj->parent->UpdateBoundingBox();

			if (quadtree->experimental && test_camera_obj != obj)
			{
				RedoQuatree();
			}
			obj->flags &= ~ProcessTransformUpdate;
		}

		Canvas* canvas = (Canvas*)obj->GetComponent(Component::Type::UI_Element, UI_Element::Type::CANVAS);
		if (canvas != nullptr)
		{
			canvas->Draw();
		}
	}

	switch (state)
	{
	case EDIT:
		//Do edit things
		break;
	case START:
		//SaveScene
		state = PLAY;
		App->editor->tab_viewport->camera = test_camera;
		//App->editor->tab_viewport->OnCameraUpdate();
		break;
	case PLAY:
		//App->Unpause();
		//Enjoy your game
		break;
	case PAUSE:
		//App->Pause();
		break;
	case STOP:
		//App->Unpause();
		//LoadScene
		state = EDIT;
		App->editor->tab_viewport->camera = viewport_camera;
		//App->editor->tab_viewport->OnCameraUpdate();

		break;
	default:
		state = EDIT;
		break;
	}
	return true;
}

void ModuleScene::RedoQuatree()
{
	quadtree->Clear();

	for (GameObject* obj : gameObjects)
	{
		quadtree->AddGameObject(obj);
	}
}

bool ModuleScene::CleanUp()
{
	LOG("Unloading Main scene");

	for (uint i = 0; i < gameObjects.size(); ++i)
	{
		RELEASE(gameObjects[i]);
	}
	gameObjects.clear();

	return true;
}

bool ModuleScene::Draw()
{
	// ALL this need a big rework
	//quadtree->ResetCullingState();

	// Cheap fix
	glColor3ub(255, 255, 255);

	Color c;
	std::vector< GameObject*> candidates;
	quadtree->CollectCandidates(candidates, test_camera->frustum);

	for ( GameObject* obj : candidates)
	{

		c = (!obj->HasChilds()) ? App->scene_base->aabb_color : Color::cyan1;
		// Just boxes colors things
		//obj->is_drawn = true;

		// Draw GameObjects
		if (!App->scene_base->camera_culling || test_camera->frustum.DO_Intersects(obj->aabb))
		{
			//test_camera->frustum.Intersects()
			glPushMatrix();
			glMultMatrixf(obj->GetGlobalTransform().Transposed().ptr());

			ComponentRenderer* renderer = obj->GetComponent<ComponentRenderer>();
			if (renderer != nullptr && renderer->IsActive())
			{
				if (renderer->show_wireframe || App->scene_base->show_all_wireframe) //wireframe
				{
					glColor3ub(App->scene_base->wireframe_color.r*255.0f, App->scene_base->wireframe_color.g * 255.0f, App->scene_base->wireframe_color.b * 255.0f);
					glLineWidth(App->scene_base->wireframe_width);
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glColor3ub(255, 255, 255);
				}
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				
				renderer->Draw();
			}
			glPopMatrix();
		}
		else
		{
			obj->is_drawn = false;
		}
	}

	for (GameObject* obj : gameObjects)
	{		
		glEnableClientState(GL_VERTEX_ARRAY);

		// AABB
		if ((obj->show_aabb || App->scene_base->show_all_aabb) && obj->aabb_VBO != 0)
		{
			c = (obj->is_drawn) ? App->scene_base->aabb_color : Color::LightGray;

			glColor3ub(c.r * 255.0f, c.g * 255.0f, c.b * 255.0f);
			glLineWidth(App->scene_base->aabb_width);

			glBindBuffer(GL_ARRAY_BUFFER, obj->aabb_VBO);
			glVertexPointer(3, GL_FLOAT, 0, NULL);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aabb_IBO);
			glDrawElements(GL_LINES, sizeof(aabb_indices), GL_UNSIGNED_INT, nullptr);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glColor3ub(255, 255, 255);
		}
		// OBB
		if ((obj->show_obb || App->scene_base->show_all_obb) && obj->obb_VBO != 0)
		{
			c = (obj->is_drawn) ? App->scene_base->obb_color : Color::DarkSlateGray;

			glColor3ub(c.r * 255.0f, c.g * 255.0f, c.b * 255.0f);
			glLineWidth(App->scene_base->obb_width);

			glBindBuffer(GL_ARRAY_BUFFER, obj->obb_VBO);
			glVertexPointer(3, GL_FLOAT, 0, NULL);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aabb_IBO);
			glDrawElements(GL_LINES, sizeof(aabb_indices), GL_UNSIGNED_INT, nullptr);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glColor3ub(255, 255, 255);
		}

		glDisableClientState(GL_VERTEX_ARRAY);

		//obj->is_drawn = true;
	}
	return true;
}

GameObject* ModuleScene::FindById(uint id)
{
	for (uint i = 0; i < gameObjects.size(); i++)
		if (gameObjects[i]->GetUID() == id) return gameObjects[i];

	return nullptr;
}

GameObject* ModuleScene::CreateGameObject(const char* name, GameObject* parent, bool visible)
{
	is_creating = true;

	if (parent == nullptr) parent = root_object;

	GameObject* obj = new GameObject(name, parent);

	if (visible)
		obj->AddComponent(Component::Type::Renderer);

	gameObjects.push_back(obj);

	return obj;
}


void ModuleScene::DeleteGameObject(GameObject* obj)
{
	// If any child of the game object is the selected -> unselect
	if (selected_gameobj == obj)
		selected_gameobj = nullptr;

	// Actual deletion of gameobject
	if (!obj->HasChilds())
	{
		// Delete from scene gameobjects
		for (uint i = 0; i < gameObjects.size(); i++)
		{
			if (gameObjects[i] == obj)
			{
				gameObjects.erase(gameObjects.begin() + i);
				break;
			}
		}
		// Deleted from parent inside destructor
		RELEASE(obj);

	}
	// Delete childs
	else
	{
		for (int i = obj->childs.size() - 1; i >= 0; i--)
		{
			DeleteGameObject(obj->childs[i]);
		}
			
		obj->childs.clear();
		DeleteGameObject(obj);
	}
}

void ModuleScene::DeleteSelected()
{
	for (int i = gameObjects.size()-1; i >= 0; i--)
	{
		if (gameObjects[i]->is_selected) 
			DeleteGameObject(gameObjects[i]);
		
	}
}

GameObject * ModuleScene::PickFromRay(Ray ray)
{
	quadtree->CollectCandidates(pick_candidates, ray);
	std::map<float, GameObject*>::iterator it = pick_candidates.begin();

	// candidates are in order, first to be picked will be the closest one
	for (;it != pick_candidates.end(); it++)
	{
		ComponentMesh* c_mesh = it->second->GetComponent<ComponentMesh>();
		if (!c_mesh) continue;

		ResourceMesh* mesh = c_mesh->GetMesh();
		if (!mesh) continue;
		
		Ray tmpRay = ray;
		tmpRay.Transform(it->second->GetGlobalTransform().Inverted());

		for (uint i = 0; i < mesh->num_indices; i += 3)
		{
			float3 one	 = mesh->vertices[mesh->indices[i]];
			float3 two	 = mesh->vertices[mesh->indices[i+1]];
			float3 three = mesh->vertices[mesh->indices[i+2]];

			if (tmpRay.Intersects(Triangle(one,two,three), nullptr, nullptr))
			{
				UnSelectAll();
				it->second->Select();
				return it->second;
			}
		}
		
	}

	return nullptr;
}

void ModuleScene::UnSelectAll(GameObject* keep_selected)
{
	if (keep_selected == nullptr) {

		for (GameObject* i : gameObjects)
		{
			i->UnSelect();
		}
	}
	else {

		for (GameObject* i : gameObjects)
		{
			if (i->GetUID() == keep_selected->GetUID())
				continue;
			i->UnSelect();
		}
	}

}