#include "PlotCtrl.h"
#include "MobiView2.h"

#include "model_application.h"

PlotCtrl::PlotCtrl(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this);
	
	index_list[0] = &index_list1;
	index_list[1] = &index_list2;
	index_list[2] = &index_list3;
	index_list[3] = &index_list4;
	index_list[4] = &index_list5;
	index_list[5] = &index_list6;
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_list[idx]->Hide();
		index_list[idx]->Disable();
		index_list[idx]->WhenSel << [this]() { re_plot(); };
		index_list[idx]->MultiSelect();
		index_list[idx]->AddColumn("(no name)");
	}
	
	time_step_slider.Range(10); //To be overwritten later.
	time_step_slider.SetData(0);
	time_step_slider.Hide();
	time_step_edit.Hide();
	time_step_slider.WhenAction << THISBACK(time_step_slider_event);
	time_step_edit.WhenAction << THISBACK(time_step_edit_event);
	
	plot_major_mode.SetData(0);
	plot_major_mode.Disable();
	plot_major_mode.WhenAction << THISBACK(plot_change);
	
	time_intervals.SetData(0);
	time_intervals.Disable();
	time_intervals.WhenAction << THISBACK(plot_change);
	
	aggregation.SetData(0);
	aggregation.Disable();
	aggregation.WhenAction << THISBACK(plot_change);
	
	scatter_inputs.Disable();
	scatter_inputs.WhenAction << THISBACK(plot_change);
	
	y_axis_mode.SetData(0);
	y_axis_mode.Disable();
	y_axis_mode.WhenAction << THISBACK(plot_change);
}

void PlotCtrl::time_step_slider_event() {
	//TODO
}

void PlotCtrl::time_step_edit_event() {
	//TODO
}

void PlotCtrl::plot_change() {
	//TODO
}

void PlotCtrl::re_plot(bool caused_by_run) {
	//TODO
}

void PlotCtrl::build_time_intervals_ctrl() {
	//TODO
}

void PlotCtrl::clean() {
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_list[idx]->Clear();
		index_list[idx]->Hide();
	}
	index_sets.clear();
	main_plot.clean();
}

void PlotCtrl::build_index_set_selecters(Model_Application *app) {
	if(!app) return;
	auto model = app->model;
	
	//NOTE: this relies on all the index sets being registered in the "global module":
	index_sets.resize(MAX_INDEX_SETS, invalid_entity_id);
	int idx = 0;
	for(auto index_set_id : model->modules[0]->index_sets) {
		auto index_set = model->find_entity<Reg_Type::index_set>(index_set_id);
		
		index_list[idx]->HeaderTab(0).SetText(str(index_set->name));
		
		for(String_View index_name : app->index_names[index_set_id.id])
			index_list[idx]->Add(str(index_name));
		
		index_list[idx]->GoBegin();
		index_list[idx]->Show();
		
		index_sets[idx] = index_set_id;
		
		++idx;
		
		if(idx == MAX_INDEX_SETS) {
			parent->log("Model has more index sets than are supported by MobiView2!", true);
			break;
		}
	}
}
	
void PlotCtrl::get_plot_setup(Plot_Setup &ps) {
	ps.selected_results.clear();
	ps.selected_series.clear();
	ps.selected_indexes.clear();
	ps.selected_indexes.resize(MAX_INDEX_SETS);
	ps.index_set_is_active.clear();
	ps.index_set_is_active.resize(MAX_INDEX_SETS, false);
	
	if(time_intervals.IsEnabled()) {
		ps.aggregation_period = (Aggregation_Period)(int)time_intervals.GetData();
		ps.aggregation_type   = (Aggregation_Type)(int)aggregation.GetData();
	} else
		ps.aggregation_period = Aggregation_Period::none;
	
	ps.major_mode = (Plot_Major_Mode)(int)plot_major_mode.GetData();
	
	if(y_axis_mode.IsEnabled())
		ps.y_axis_mode = (Y_Axis_Mode)(int)y_axis_mode.GetData();
	else
		ps.y_axis_mode = Y_Axis_Mode::regular;
	
	ps.scatter_inputs = (scatter_inputs.IsEnabled() && (bool)scatter_inputs.Get());
	
	if(!parent->model_is_loaded()) return;
	
	Vector<int> sel_inputs = parent->input_selecter.GetSel();
	for(int idx : sel_inputs) {
		Upp::Ctrl *ctrl = ~parent->input_selecter.GetNode(idx).ctrl;
		if(!ctrl) continue;
		ps.selected_series.push_back(reinterpret_cast<Entity_Node *>(ctrl)->var_id);
	}
	Vector<int> sel_results = parent->result_selecter.GetSel();
	for(int idx : sel_results) {
		Upp::Ctrl *ctrl = ~parent->result_selecter.GetNode(idx).ctrl;
		if(!ctrl) continue;
		ps.selected_results.push_back(reinterpret_cast<Entity_Node *>(ctrl)->var_id);
	}
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(!index_list[idx]->IsVisible()) continue;
		int row_count = index_list[idx]->GetCount();
		for(int row = 0; row < row_count; ++row) {
			if(!index_list[idx]->IsSelected(row)) continue;
			ps.selected_indexes[idx].push_back(Index_T { index_sets[idx], row });
		}
	}
	
	for(Var_Id var_id : ps.selected_series) {
		const std::vector<Entity_Id> &var_index_sets = var_id.type == 1 ? parent->app->series_data.get_index_sets(var_id) : parent->app->additional_series_data.get_index_sets(var_id);
		for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
			if(std::find(var_index_sets.begin(), var_index_sets.end(), index_sets[idx]) != var_index_sets.end())
				ps.index_set_is_active[idx] = true;
		}
	}
	for(Var_Id var_id : ps.selected_results) {
		const std::vector<Entity_Id> &var_index_sets = parent->app->result_data.get_index_sets(var_id);
		for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
			if(std::find(var_index_sets.begin(), var_index_sets.end(), index_sets[idx]) != var_index_sets.end())
				ps.index_set_is_active[idx] = true;
		}
	}
}

void recursive_select(TreeCtrl &tree, int node, std::vector<Var_Id> &select) {
	Upp::Ctrl *ctrl = ~tree.GetNode(node).ctrl;
	if(ctrl) {
		Var_Id var_id = reinterpret_cast<Entity_Node *>(ctrl)->var_id;
		if(std::find(select.begin(), select.end(), var_id) != select.end())
			tree.SelectOne(node, true);
	}
	for(int child_idx = 0; child_idx < tree.GetChildCount(node); ++child_idx) {
		int child = tree.GetChild(node, child_idx);
		recursive_select(tree, child, select);
	}
}

void PlotCtrl::set_plot_setup(Plot_Setup &ps) {
	plot_major_mode.SetData((int)ps.major_mode);
	scatter_inputs.SetData((int)ps.scatter_inputs);
	y_axis_mode.SetData((int)ps.y_axis_mode);
	time_intervals.SetData((int)ps.aggregation_period);
	aggregation.SetData((int)ps.aggregation_type);
	time_step_slider.SetData((int)ps.profile_time_step);
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_list[idx]->ClearSelection(false);
		int row_count = index_list[idx]->GetCount();
		for(int row = 0; row < row_count; ++row) {
			Index_T index = { index_sets[idx], row };
			if(std::find(ps.selected_indexes[idx].begin(), ps.selected_indexes[idx].end(), index) != ps.selected_indexes[idx].end())
				index_list[idx]->Select(row);
		}
	}
	
	parent->result_selecter.ClearSelection();
	recursive_select(parent->result_selecter, 0, ps.selected_results);
	
	parent->input_selecter.ClearSelection();
	recursive_select(parent->input_selecter, 0, ps.selected_series);
	
	plot_change();
}