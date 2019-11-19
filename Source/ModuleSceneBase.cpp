#include "Globals.h"
#include "Application.h"
#include "ModuleSceneBase.h"
#include "ModuleResources.h"
#include "Hierarchy.h"
#include "Viewport.h"
#include "GameObject.h"

#include "glew/include/GL/glew.h"
#include "par_shapes.h"
#include "mmgr/mmgr.h"

GameObject* ModuleSceneBase::main_camera_object;
ComponentCamera* ModuleSceneBase::main_camera;

ModuleSceneBase::ModuleSceneBase(bool start_enabled) : Module("SceneIntro", start_enabled)
{}

ModuleSceneBase::~ModuleSceneBase()
{}

// Load assets
bool ModuleSceneBase::Start(Config* config)
{
	//main_camera->Move(vec3(0, 7.5f, 7.5f));
	//main_camera->LookAt(vec3(0, 0, 0));

	return true;
}



// Update
bool ModuleSceneBase::Update(float dt)
{
	UpdateMainCamera(dt);

	return true;
}

bool ModuleSceneBase::PostUpdate(float dt)
{

	return true;
}

void ModuleSceneBase::UpdateMainCamera(float dt)
{

	if (main_camera == nullptr || main_camera->viewport_focus == false) return;

	// TODO Change library glmath to math

	// Camera ZOOM with MOUSE WHEEL
	ImGuiIO io = ImGui::GetIO();
	vec3 newPos(0, 0, 0);
	float speed = io.MouseWheel * zoom_speed * dt; // TODO Not hardcoded speed
	if (speed != 0)
	{
		if (App->input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT || App->input->GetKey(SDL_SCANCODE_RSHIFT) == KEY_REPEAT)
			speed *= 2.0f;

		newPos -= main_camera->Z * speed;
		main_camera->Position += newPos;
		main_camera->update_projection = true;
	}

	// Center main_camera to selected gameobject with F
	if (App->input->GetKey(SDL_SCANCODE_F) == KEY_DOWN)
	{
		LOG("Centering viewport camera from [%f,%f,%f]", main_camera->Position.x, main_camera->Position.y, main_camera->Position.z, 'v');
		// To change to the Reference we want to orbit at
		GameObject* object = App->scene->GetSelectedGameObject();
		if (object != nullptr)
		{
			float3 c = { main_camera->Position.x,main_camera->Position.y,main_camera->Position.z };
			float3 p = object->bounding_box[9] - c;

			float l = length(vec3(p.x, p.y, p.z));
			float min = l;
			int face = 9;

			for (int i = 10; i < 13; i++)
			{
				p = object->bounding_box[i] - c;
				l = length(vec3(p.x, p.y, p.z));
				if (l < min) {
					min = l;
					face = i;
				}
			}

			vec3 new_p = { object->bounding_box[face].x,object->bounding_box[face].y,object->bounding_box[face].z };
			float size = object->size.MaxElement();
			float offset = Sqrt((size*size) - (size*size / 4));
			float parent = (object->HasChilds()) ? 1.0f : -1.0f;

			switch (face)
			{
			case 9:
				main_camera->Position = new_p + (main_camera->c_Z * offset * parent);
				break;
			case 10:
				main_camera->Position = new_p + (main_camera->c_X * offset* parent);
				break;
			case 11:
				main_camera->Position = new_p - (main_camera->c_X * offset* parent);
				break;
			case 12:
				main_camera->Position = new_p - (main_camera->c_Z * offset* parent);
				break;
			default:
				LOG("Could not detect closest face", 'e');
				break;
			}
			//mesh->bounding_box[13] = { Position.x, Position.y, Position.z };
			//App->resources->bbox_indices[25] = face;
			//App->resources->GenBoundingBox(mesh);

			LOG("FACE %i", face, 'v');
			LOG("To [%f,%f,%f]", main_camera->Position.x, main_camera->Position.y, main_camera->Position.z, 'v');
			LOG("Looking at [%f,%f,%f]", new_p.x, new_p.y, new_p.z, 'v');

			LookAt(new_p);

			main_camera->update_projection = true;
		}
	}
	//Free move 
	if (App->input->GetMouseButton(SDL_BUTTON_RIGHT) == KEY_REPEAT)
	{
		//Camera motion with WASD, Shift to go faster
		vec3 newPos(0, 0, 0);
		float speed = 10.0f * dt;

		if (App->input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT ||
			App->input->GetKey(SDL_SCANCODE_RSHIFT) == KEY_REPEAT)
			speed *= 2.0f;

		if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) newPos -= main_camera->Z * speed;
		if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) newPos += main_camera->Z * speed;


		if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) newPos -= main_camera->X * speed;
		if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) newPos += main_camera->X * speed;

		if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_LCTRL) == KEY_REPEAT) newPos -= main_camera->Y * speed;
		else if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT) newPos += main_camera->Y * speed;

		main_camera->Position += newPos;
		main_camera->Reference += newPos;

		RotateWithMouse();
		main_camera->update_projection = true;
	}

	// Orbit move
	if (App->input->GetMouseButton(SDL_BUTTON_LEFT) == KEY_REPEAT && App->input->GetKey(SDL_SCANCODE_LALT))
	{
		RotateWithMouse();
		// To change to the Reference we want to orbit at
		GameObject* object = App->scene->GetSelectedGameObject();
		if (object != nullptr)
		{
			LookAt({ object->center.x, object->center.y,object->center.z });
			main_camera->update_projection = true;
		}

	}

	// Recalculate matrix -------------
	main_camera->CalculateViewMatrix();

	

}

// -----------------------------------------------------------------
void ModuleSceneBase::Look(const vec3 &Position, const vec3 &Reference, bool RotateAroundReference)
{
	main_camera->Position = Position;
	main_camera->Reference = Reference;

	main_camera->Z = normalize(Position - Reference);
	main_camera->X = normalize(cross(vec3(0.0f, 1.0f, 0.0f), main_camera->Z));
	main_camera->Y = cross(main_camera->Z, main_camera->X);
	
	if (!RotateAroundReference)
	{
		main_camera->Reference = main_camera->Position;
		main_camera->Position += main_camera->Z * 0.05f;
	}

	main_camera->CalculateViewMatrix();
}

// -----------------------------------------------------------------
void ModuleSceneBase::LookAt(const vec3 &Spot)
{
	main_camera->Reference = Spot;

	main_camera->Z = normalize(main_camera->Position - main_camera->Reference);
	main_camera->X = normalize(cross(vec3(0.0f, 1.0f, 0.0f), main_camera->Z));
	main_camera->Y = cross(main_camera->Z, main_camera->X);

	main_camera->CalculateViewMatrix();
}


// -----------------------------------------------------------------
void ModuleSceneBase::Move(const vec3 &Movement)
{
	main_camera->Position += Movement;
	main_camera->Reference += Movement;

	main_camera->CalculateViewMatrix();
}

void ModuleSceneBase::RotateWithMouse() {
	// Camera rotation with mouse
	int dx, dy;
	App->input->GetMouseMotion(dx, dy);
	dx = -dx;
	dy = -dy;

	float Sensitivity = 0.25f;

	main_camera->Position -= main_camera->Reference;

	if (dx != 0)
	{
		float DeltaX = (float)dx * Sensitivity;

		main_camera->X = rotate(main_camera->X, DeltaX, vec3(0.0f, 1.0f, 0.0f));
		main_camera->Y = rotate(main_camera->Y, DeltaX, vec3(0.0f, 1.0f, 0.0f));
		main_camera->Z = rotate(main_camera->Z, DeltaX, vec3(0.0f, 1.0f, 0.0f));
	}

	if (dy != 0)
	{
		float DeltaY = (float)dy * Sensitivity;

		main_camera->Y = rotate(main_camera->Y, DeltaY, main_camera->X);
		main_camera->Z = rotate(main_camera->Z, DeltaY, main_camera->X);

		if (main_camera->Y.y < 0.0f)
		{
			main_camera->Z = vec3(0.0f, main_camera->Z.y > 0.0f ? 1.0f : -1.0f, 0.0f);
			main_camera->Y = cross(main_camera->Z, main_camera->X);
		}
	}
	main_camera->Position = main_camera->Reference + main_camera->Z * length(main_camera->Position);
}

bool ModuleSceneBase::CleanUp()
{
	LOG("Unloading Intro scene");

	return true;
}


bool ModuleSceneBase::Draw()
{
	if (App->editor->show_plane) // plane
		DrawGridPlane();

	if (App->editor->show_axis) // axis
		DrawAxis();

	return true;
}



void ModuleSceneBase::DrawGridPlane()
{
	glLineWidth(0.25f);

	glBegin(GL_LINES);
	glColor3ub(100, 100, 100);
	float d = plane_length;
	for (float i = -d; i <= d; ++i)
	{
		glVertex3f(i, 0.f, 0.f);
		glVertex3f(i, 0, d);

		glVertex3f(0.f, 0.f, i);
		glVertex3f(d, 0, i);

		glVertex3f(i, 0.f, 0.f);
		glVertex3f(i, 0, -d);

		glVertex3f(0.f, 0.f, i);
		glVertex3f(-d, 0, i);
	}
	glEnd();
}

void ModuleSceneBase::DrawAxis()
{
	glLineWidth(3.0f);
	glBegin(GL_LINES);

	// x
	glColor3ub(255, 0, 0);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(axis_length, 0.0f, 0.0f);

	//y
	glColor3ub(0, 255, 0);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, axis_length, 0.0f);

	// z
	glColor3ub(0, 0, 255);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, axis_length);

	glColor3ub(255, 255, 255);


	glEnd();
}