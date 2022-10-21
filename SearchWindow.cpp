#include "SearchWindow.h"
#include "MobiView2.h"


SearchWindow::SearchWindow(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "MobiView2 parameter search");
	Sizeable();
	search_field.WhenAction = THISBACK(find);
	result_field.AddColumn("Parameter");
	result_field.AddColumn("Group");
	result_field.AddColumn("Module");
	
	result_field.WhenSel = THISBACK(select_item);
}

void SearchWindow::find() {
	if(!parent->model_is_loaded())
		return;

	result_field.Clear();
	par_list.clear();
	
	std::string match = search_field.GetData().ToStd();
	std::transform(match.begin(), match.end(), match.begin(), ::tolower);  //TODO: trim leading whitespace
	
	// Hmm, this is a bit cumbersome. See also same in model_application.cpp
	for(int idx = -1; idx < parent->model->modules.count(); ++idx) {
		Entity_Id module_id = invalid_entity_id;
		String mod_name = "";
		if(idx >= 0) {
			module_id = { Reg_Type::module, idx };
			auto mod = parent->model->modules[module_id];
			if(!mod->has_been_processed) continue;
			mod_name = mod->name.data();
		}
		for(auto group_id : parent->model->by_scope<Reg_Type::par_group>(module_id)) {
			auto group = parent->model->par_groups[group_id];
			for(auto par_id : group->parameters) {
				auto par = parent->model->parameters[par_id];
				std::string par_name = par->name;
				std::transform(par_name.begin(), par_name.end(), par_name.begin(), ::tolower);
				size_t pos = par_name.find(match);
				
				if(pos != std::string::npos) {
					result_field.Add(par->name.data(), mod_name, group->name.data());
					par_list.push_back( {group_id, par_id} );
				}
			}
		}
	}
}

bool
select_group_recursive(TreeCtrl &tree, int node, Entity_Id group_id) {
	Upp::Ctrl *ctrl = ~tree.GetNode(node).ctrl;
	if(ctrl) {
		if(group_id == reinterpret_cast<Entity_Node *>(ctrl)->entity_id) {
			tree.SetFocus();
			tree.SetCursor(node);
			return true;
		}
	}
	for(int idx = 0; idx < tree.GetChildCount(node); ++idx) {
		if(select_group_recursive(tree, tree.GetChild(node, idx), group_id))
			return true;
	}
	return false;
}

void
SearchWindow::select_item() {
	int row = result_field.GetCursor();
	if(row < 0) return;
	
	auto sel_group = par_list[row].first;
	auto sel_par   = par_list[row].second;

	bool success = select_group_recursive(parent->par_group_selecter, 0, sel_group);
	
	if(success) {
		int row = 0;
		for(auto &par_data : parent->params.listed_pars) {
			if(sel_par == par_data.id) break;
			++row;
		}
		ArrayCtrl &par_view = parent->params.parameter_view;
		if(row < par_view.GetCount()) {
			par_view.SetFocus();
			par_view.SetCursor(row);
		}
	}
}
