#include "Hierarchy.h"
#include "Globals.h"
#include "ImGui/imgui.h"
#include "mmgr/mmgr.h"

char Hierarchy::scene_name[MAX_NAME_LENGTH];

// List of nodes to start drawing
std::vector<HierarchyNode*> Hierarchy::root_nodes;
// List of all nodes (NOT used to draw)
std::vector<HierarchyNode*> Hierarchy::nodes;
// Temporary list of selected nodes
std::vector<HierarchyNode*> Hierarchy::selected_nodes;


// ---------------------------------------------------------
Hierarchy::Hierarchy() : Panel("Hierarchy")
{
	width = default_width;
	height = default_height;
	pos_x = default_pos_x;
	pos_y = default_pos_y;

	//-----------
	SetSceneName("Recursively");

	for (int i = 0; i < 6; i++) {
		AddNode();
	}
	AddNode();
	//-----------
}

Hierarchy::~Hierarchy()
{
	for (int i = 0; i < nodes.size(); i++)
	{
		DeleteNode(nodes[i]);
	}
}

// Add a new dummy node, child of the passed node or root node if parent is nullptr
// Returns pointer to the added node
HierarchyNode* Hierarchy::AddNode(HierarchyNode* parent)
{
	HierarchyNode* n = new HierarchyNode(parent);

	if (parent == nullptr)
	{
		root_nodes.push_back(n);
		n->LogAction("Added root");
	}
	else
	{
		parent->childs.push_back(n);
		parent->flags &= ~HierarchyNode::leaf_flags;
		parent->flags |= HierarchyNode::base_flags;
		//LOG("Added child '%s', id: %ld to parent '%s', id: %ld",n->name, n->id, parent->name, parent->id,'d');
		n->LogAction("Added child");
		parent->LogAction("to parent");

	}

	// General list of all nodes, not used for printing
	nodes.push_back(n);
	return n;

}

bool SearchAndDeleteNode(HierarchyNode* n, std::vector<HierarchyNode*>& v)
{
	if (n == nullptr) return false;

	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] != nullptr && v[i]->id == n->id)
		{
			v.erase(v.begin()+i);
			return true;
		}
	}
	return false;
}

void Hierarchy::DeleteNode(HierarchyNode* n)
{
	// if has no childs, delete the node
	if (n->childs.empty())
	{
		SearchAndDeleteNode(n, root_nodes);
		SearchAndDeleteNode(n, nodes);

		n->childs.clear();

		if (n->parent)
		{
			SearchAndDeleteNode(n, n->parent->childs);

			if (n->parent->childs.empty())
			{
				n->parent->flags = HierarchyNode::leaf_flags;
			}
		}
		n->LogAction("Deleted");

		delete(n);
	}
	else
	{
		// Delete childs before the actual node
		for (int i = 0; i < n->childs.size(); i++)
		{
			DeleteNode(n->childs[i]);
		}

		n->childs.clear();
		DeleteNode(n);
	}
}

void Hierarchy::DeleteSelected()
{
	for (HierarchyNode* n : selected_nodes)
	{
		DeleteNode(n);
	}
	selected_nodes.clear();
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
			if (want_rename || ImGui::Button("Apply"))
			{
				node->SetName(buffer);
				ImGui::CloseCurrentPopup();
			}
			//ImGui::Text("Option #2");
			//ImGui::Text("Option #3");
			ImGui::EndPopup();
		}

		// if treenode is clicked, check whether it is a single or multi selection
		if (ImGui::IsItemClicked())
		{
			if (!ImGui::GetIO().KeyCtrl) // Single selection, clear selected nodes
			{
				for (HierarchyNode* i : Hierarchy::selected_nodes)
				{ // Selected nodes has selected state, need to unselect, toggle is safe [panaderia de pan]
					i->ToggleSelection();
				}
				Hierarchy::selected_nodes.clear();
				LOG("Single selection", 'v');
			}
			else
			{
				LOG("Multi Selection", 'v');
			}

			// Always need to toggle the state of selection of the node, getting its current state
			if (node->ToggleSelection())
			{
				Hierarchy::selected_nodes.push_back(node);
			}
			else
			{
				SearchAndDeleteNode(node, Hierarchy::selected_nodes);
				//Hierarchy::selected_nodes.erase(Hierarchy::selected_nodes.find(node));
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

void Hierarchy::Draw()
{
	if (ImGui::Button("Add Free Node"))
		AddNode();
	ImGui::SameLine();

	if (ImGui::Button("Add Childs"))
	{
		for (HierarchyNode* selected : selected_nodes)
		{
			AddNode(selected);
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Delete"))
		DeleteSelected();

	ImGui::Text(scene_name);
	ImGui::Separator();

	// Actually drawing of nodes, recursively, needs only parents set
	DrawNodes(root_nodes);
}

bool Hierarchy::SetSceneName(const char * name)
{
	if (strlen(name) < MAX_NAME_LENGTH)
	{
		LOG("Renaming scene from '%s' to '%s'", scene_name, name);
		sprintf_s(scene_name, 512, "%s", name);
		return true;
	}
	else
	{
		LOG("Scene name exceeds max length (%d)", MAX_NAME_LENGTH, 'e');
		return false;

	}
}