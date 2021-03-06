#include "Globals.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "Config.h"

#include "mmgr/mmgr.h"

ModuleWindow::ModuleWindow(bool start_enabled) : Module("Window", start_enabled)
{
}

// Destructor
ModuleWindow::~ModuleWindow()
{
}

// Called before render is available
bool ModuleWindow::Init(Config* config)
{
	LOG("Init SDL window & surface");
	bool ret = true;

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		SDL_DisplayMode DM;
		SDL_GetDesktopDisplayMode(0, &DM);

		//Create window
		screen_width = config->GetNumber("Width", 1024);
		screen_height = config->GetNumber("Height", 768);

		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

		//Use OpenGL 3.2
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		fullscreen = config->GetBool("Fullscreen", false);
		resizable = config->GetBool("Resizable", true);
		borderless = config->GetBool("Borderless", false);
		fullscreen_desktop = config->GetBool("Fullscreen Desktop", false);

		if (fullscreen == true)
			flags |= SDL_WINDOW_FULLSCREEN;

		if (resizable == true)
			flags |= SDL_WINDOW_RESIZABLE;

		if (borderless == true)
			flags |= SDL_WINDOW_BORDERLESS;

		if (fullscreen_desktop == true)
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

		window = SDL_CreateWindow(App->GetAppName(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, flags);

		if (window == nullptr)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError(),'d');
			ret = false;
		}
		else
		{
			//Get window surface
			screen_surface = SDL_GetWindowSurface(window);
		}
	}
	return ret;
}

bool ModuleWindow::Start(Config* config)
{
	//SetDefaultIcon();

	//std::string icon_file = config->GetString("Icon", "");
	//if (icon_file.size() > 1)
	//	SetIcon(icon_file.c_str());

	SetBrightness(config->GetNumber("Brightness", 1.0f));

	// Force to trigger a chain of events to refresh aspect ratios	
	SDL_SetWindowSize(window, screen_width, screen_height);

	return true;
}

// Called before quitting
bool ModuleWindow::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	//Destroy window
	if(window != nullptr)
		SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();
	return true;
}

void ModuleWindow::Save(Config* config) const
{
	//config->AddString("Icon", icon_file.c_str());
	config->SetNumber("Brightness", GetBrightness());
	config->SetNumber("Width", GetWidth());
	config->SetNumber("Height", this->GetHeight());
	config->SetBool("Fullscreen", IsFullscreen());
	config->SetBool("Resizable", IsResizable());
	config->SetBool("Borderless", IsBorderless());
	config->SetBool("Fullscreen Desktop", IsFullscreenDesktop());
}

void ModuleWindow::Load(Config* config)
{
	//SetIcon(config->GetString("Icon", ""));
	SetBrightness(config->GetNumber("Brightness", 1.0f));
	SetWidth(config->GetNumber("Width", 1024));
	SetHeigth(config->GetNumber("Height", 768));
	SetFullscreen(config->GetBool("Fullscreen", false));
	SetResizable(config->GetBool("Resizable", true));
	SetBorderless(config->GetBool("Borderless", false));
	SetFullScreenDesktop(config->GetBool("Fullscreen Desktop", false));
}

uint ModuleWindow::GetRefreshRate() const
{
	uint ret = 0;

	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
	{
		LOG("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
	}
	else
		ret = dm.refresh_rate;

	return ret;
}

void ModuleWindow::GetMaxMinSize(uint & min_width, uint & min_height, uint & max_width, uint & max_height) const
{
	min_width = 480;
	min_height = 320;
	max_width = 3000;
	max_height = 2000;

	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
	{
		LOG("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
	}
	else
	{
		max_width = dm.w;
		max_height = dm.h;
	}
}

void ModuleWindow::SetFullscreen(bool set)
{
	if (set != fullscreen)
	{
		fullscreen = set;
		if (fullscreen == true)
		{
			if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0)
				LOG("Could not switch to fullscreen: %s\n", SDL_GetError());
			fullscreen_desktop = false;
			SDL_Log("this is a test");
		}
		else
		{
			if (SDL_SetWindowFullscreen(window, 0) != 0)
				LOG("Could not switch to windowed: %s\n", SDL_GetError());
		}
	}
}

void ModuleWindow::SetResizable(bool set)
{
	// cannot be changed while the program is running, but we can save the change
	resizable = set;
}

void ModuleWindow::SetBorderless(bool set)
{
	if (set != borderless && fullscreen == false && fullscreen_desktop == false)
	{
		borderless = set;
		SDL_SetWindowBordered(window, (SDL_bool)!borderless);
	}
}

void ModuleWindow::SetFullScreenDesktop(bool set)
{
	if (set != fullscreen_desktop)
	{
		fullscreen_desktop = set;
		if (fullscreen_desktop == true)
		{
			if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
				LOG("Could not switch to fullscreen desktop: %s\n", SDL_GetError());
			fullscreen = false;
		}
		else
		{
			if (SDL_SetWindowFullscreen(window, 0) != 0)
				LOG("Could not switch to windowed: %s\n", SDL_GetError());
		}
	}
}

void ModuleWindow::SetBrightness(float set)
{
	CAP(set);
	if (SDL_SetWindowBrightness(window, set) != 0)
		LOG("Could not change window brightness: %s\n", SDL_GetError());
}

//void ModuleWindow::SetIcon(const char * file)
//{
//	if (file != nullptr && file != icon_file)
//	{
//		icon_file = file;
//
//		SDL_Surface *surface = SDL_LoadBMP_RW(App->file_system->Load(file), 1);
//		SDL_SetWindowIcon(window, surface);
//		SDL_FreeSurface(surface);
//	}
//}
