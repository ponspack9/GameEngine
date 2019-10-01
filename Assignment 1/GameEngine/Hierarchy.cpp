#include "Hierarchy.h"
#include "Console.h"
#include "Globals.h"
#include "ImGui/imgui.h"
//Hierarchy hierarchy = Hierarchy();

char Hierarchy::scene_name[MAX_NAME_LENGTH];

std::vector<HierarchyNode*> Hierarchy::nodes;
std::set<HierarchyNode*> Hierarchy::selected_nodes;

Hierarchy::Hierarchy()
{
}

void Hierarchy::Init() 
{
	SetSceneName("Recursively");

	/*for (int i = 0; i < 6; i++) {
		AddNode();
		for (int j = 0; j < i; j++) {
			AddNode(Hierarchy::nodes.back());
		}
	}*/
	/*for (int i = 0; i < 3; i++) {
		AddNode(Hierarchy::nodes.back());
		for (int j = 0; j < i+1; j++) {
			if(Hierarchy::nodes.back()->childs.empty())
				AddNode(Hierarchy::nodes.back());
			else
				AddNode(Hierarchy::nodes.back()->childs.back());
		}
	}*/
	for (int i = 0; i < 6; i++) {
		AddNode();
		/*for (int j = 0; j < i; j++) {
			AddNode(Hierarchy::nodes.back());
		}*/
	}
	AddNode();
}

Hierarchy::~Hierarchy()
{
}

// Add a new dummy node, child of the passed node or an independent if is nullptr
void Hierarchy::AddNode(HierarchyNode* parent) 
{
	HierarchyNode* n = new HierarchyNode(HierarchyNode::leaf_flags);

	if (parent == nullptr)
	{
		Hierarchy::nodes.push_back(n);
		LOG("Added free node %ld", n->id,'d');
	}
	else {
		//Hierarchy::nodes.push_back(n);
		parent->childs.push_back(n);
		parent->flags &= ~HierarchyNode::leaf_flags;
		parent->flags |= HierarchyNode::base_flags;
		LOG("Added child %ld to parent %ld", n->id, parent->id,'d');
	}


}


void Hierarchy::ShowHierarchy(bool* open) 
{

	Draw("Hierarchy", open);
}

// Problems with the last child and parent doing the same thing
void DrawNodes(std::vector<HierarchyNode*>& v)
{
	static char buffer[512] = "";
	for (HierarchyNode* node : v) 
	{
		//TODO In future to be substituited buffer by node.name only, no need to show id
		sprintf_s(buffer, 512, "%s %ld", node->name, node->id);
		bool is_open = ImGui::TreeNodeEx(buffer, node->flags);


		// Options menu poped up when right clicking a node
		if (ImGui::BeginPopupContextItem(buffer))
		{
			sprintf_s(buffer, 512, "");

			ImGui::Text("Options");
			ImGui::Separator();
			ImGui::Text("Rename");	
			ImGui::SameLine();
	
			bool want_rename = ImGui::InputTextWithHint("", (const char*)node->name, buffer, IM_ARRAYSIZE(buffer), ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			if(want_rename || ImGui::Button("Apply"))
			{
				node->SetName(buffer);
				ImGui::CloseCurrentPopup();
			}

			ImGui::Text("Option #2");
			ImGui::Text("Option #3");
			ImGui::EndPopup();
		}

		// if treenode is clicked, check whether it is a single or multi selection
		if (ImGui::IsItemClicked()) 
		{
			if (!ImGui::GetIO().KeyCtrl) // Single selection, clear selected nodes
			{
				for (HierarchyNode* i : Hierarchy::selected_nodes)
				{ // Selected nodes has selected state, toggle is safe [panaderia de pan]
					i->ToggleSelection();
				}
				Hierarchy::selected_nodes.clear();

				LOG("Single selection", 'd');
			}
			else
			{
				LOG("Multi Selection", 'd');
			}

			// Always need to toggle the state of selection of the node, getting its current state
			if (node->ToggleSelection())
			{
				Hierarchy::selected_nodes.emplace(node);
			}
			else {
				Hierarchy::selected_nodes.erase(Hierarchy::selected_nodes.find(node));
			}

		}

		if (is_open)
		{	
			// Node is open, need to draw childs if has childs
			if (node->childs.size() > 0)
			{
				DrawNodes(node->childs);
			}


			ImGui::TreePop();
		}
	}

}

void Hierarchy::Draw(const char * title, bool * p_open)
{

	ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Add Free Node")) {
		AddNode();
	}ImGui::SameLine();

	if (ImGui::Button("Add Childs")) {
		for (HierarchyNode* selected : Hierarchy::selected_nodes)
			AddNode(selected);
	}

	ImGui::Text(Hierarchy::scene_name);
	ImGui::Separator();

	// Actually drawing of nodes
	DrawNodes(Hierarchy::nodes);

	
	ImGui::End();
}

bool Hierarchy::SetSceneName(const char * name)
{
	if (strlen(name) < MAX_NAME_LENGTH) {
		LOG("Renaming scene from '%s' to '%s'", Hierarchy::scene_name, name);
		sprintf_s(Hierarchy::scene_name, 512, "%s",name);
		return true;
	}
	else {
		LOG("Scene name exceeds max length (%d)",MAX_NAME_LENGTH,'e');
		return false;

	}
}