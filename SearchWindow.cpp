#include "SearchWindow.h"
#include "MobiView2.h"


SearchWindow::SearchWindow(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "MobiView2 parameter search");
	Sizeable();
	search_field.WhenAction = THISBACK(find);
	result_field.AddColumn("Parameter");
	result_field.AddColumn("Module");
	result_field.AddColumn("Group");
	
	// WhenSel doesn't respond if it is already selected.
	//result_field.WhenSel = THISBACK(select_item);
	result_field.WhenLeftClick = THISBACK(select_item);
}

void SearchWindow::clean() {
	search_field.Clear();
	result_field.Clear();
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
			module_id = { Reg_Type::module, (s16)idx };
			auto mod = parent->model->modules[module_id];
			//if(!mod->has_been_processed) continue;
			mod_name = mod->name.data();
		}
		for(auto group_id : parent->model->get_scope(module_id)->by_type(Reg_Type::par_group)) {
			auto group = parent->model->par_groups[group_id];
			
			if(group->decl_type == Decl_Type::option_group) continue;
			
			for(auto par_id : parent->model->get_scope(group_id)->by_type(Reg_Type::parameter)) {
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



void
SearchWindow::select_item() {
	int row = result_field.GetCursor();
	if(row < 0) return;
	
	auto sel_group = par_list[row].first;
	auto sel_par   = par_list[row].second;

	bool success = parent->select_par_group(sel_group);
	
	if(success) {
		auto pars = parent->params.get_row_parameters();
		int row = 0;
		for(auto &par : pars) {
			if(sel_par == par) break;
			++row;
		}
		ArrayCtrl &par_view = parent->params.parameter_view;
		if(row < par_view.GetCount()) {
			par_view.SetFocus();
			par_view.SetCursor(row);
		}
	}
}
