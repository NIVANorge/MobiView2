#include "PlotCtrl.h"
#include "MobiView2.h"

#include "model_application.h"


#define IMAGECLASS IconImg9
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

PlotCtrl::PlotCtrl(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this);
	
	main_plot.parent = parent;
	main_plot.plot_info = &parent->plot_info;
	
	index_list[0] = &index_list1;
	index_list[1] = &index_list2;
	index_list[2] = &index_list3;
	index_list[3] = &index_list4;
	index_list[4] = &index_list5;
	index_list[5] = &index_list6;
	
	push_sel_all[0] = &push_sel_all1;
	push_sel_all[1] = &push_sel_all2;
	push_sel_all[2] = &push_sel_all3;
	push_sel_all[3] = &push_sel_all4;
	push_sel_all[4] = &push_sel_all5;
	push_sel_all[5] = &push_sel_all6;
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_list[idx]->Hide();
		index_list[idx]->Disable();
		index_list[idx]->WhenSel << [this]() { re_plot(); };
		index_list[idx]->MultiSelect();
		index_list[idx]->AddColumn("(no name)");
		
		push_sel_all[idx]->Hide();
		push_sel_all[idx]->Disable();
		push_sel_all[idx]->SetImage(IconImg9::Add(), IconImg9::Remove());
		push_sel_all[idx]->WhenAction << [this, idx]() {
			int rows = index_list[idx]->GetCount();
			int count = index_list[idx]->GetSelectCount();
			if(rows == count) {
				index_list[idx]->Select(0, rows, false);
				index_list[idx]->Select(0);
			} else
				index_list[idx]->Select(0, rows);
		};
	}
	
	time_step_slider.Range(10); //To be overwritten later.
	time_step_slider.SetData(0);
	time_step_slider.Hide();
	time_step_edit.Hide();
	time_step_slider.WhenAction << THISBACK(time_step_slider_event);
	time_step_edit.WhenAction << THISBACK(time_step_edit_event);
	
	plot_major_mode.Add((int)Plot_Mode::regular, "Regular");
	plot_major_mode.Add((int)Plot_Mode::stacked, "Stacked");
	plot_major_mode.Add((int)Plot_Mode::stacked_share, "Stacked share");
	plot_major_mode.Add((int)Plot_Mode::histogram, "Histogram");
	plot_major_mode.Add((int)Plot_Mode::profile, "Profile");
	plot_major_mode.Add((int)Plot_Mode::profile2D, "Profile 2D");
	plot_major_mode.Add((int)Plot_Mode::compare_baseline, "Compare baseline");
	plot_major_mode.Add((int)Plot_Mode::residuals, "Residuals");
	plot_major_mode.Add((int)Plot_Mode::residuals_histogram, "Residuals histogram");
	plot_major_mode.Add((int)Plot_Mode::qq, "Quantile-Quantile");
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
	
	push_play.SetImage(IconImg9::Play());
	push_play.Hide();
	push_play.WhenAction << THISBACK(play_pushed);
	
	push_rewind.SetImage(IconImg9::Rewind());
	push_rewind.Hide();
	push_rewind.WhenAction << [this]() {
		is_playing = false;
		KillTimeCallback();
		push_play.SetImage(IconImg9::Play());
		time_step_slider.SetData(0);
		time_step_slider_event();
	};
	
	main_plot.plot_ctrl = this;
	
	static const char *month_names[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	for(int idx = 0; idx < 12; ++idx)
		pivot_month.Add(idx+1, month_names[idx]);
	pivot_month.GoBegin();
	pivot_month.WhenAction << THISBACK(plot_change);
	pivot_month.Disable();
}

void PlotCtrl::time_step_slider_event() {
	if(!parent->model_is_loaded()) return;
	
	s64 ts = time_step_slider.GetData();
	// Recompute the displayed date in the time_step_edit based on the new position of the
	// slider.
	Aggregation_Period agg = main_plot.setup.aggregation_period;
	// TODO: this functionality could maybe be factored out..
	Time_Step_Size ts_size = parent->app->time_step_size;
	if(agg == Aggregation_Period::weekly) {
		ts_size.unit = Time_Step_Size::second;
		ts_size.multiplier = 7*86400;
	} else if(agg == Aggregation_Period::monthly) {
		ts_size.unit = Time_Step_Size::month;
		ts_size.multiplier = 1;
	} else if(agg == Aggregation_Period::yearly) {
		ts_size.unit = Time_Step_Size::month;
		ts_size.multiplier = 12;
	}
	Date_Time time = advance(profile_base_time, ts_size, ts);
	time_step_edit.SetData(convert_time(time));
	main_plot.setup.profile_time_step = ts;
	main_plot.replot_profile();
}

void PlotCtrl::time_step_edit_event() {
	if(!parent->model_is_loaded()) return;
	// NOTE: currently only works for no aggregation. Wouldn't be that difficult to fix it.
	Upp::Time tm = time_step_edit.GetData();
	if(IsNull(tm)) return;
	Date_Time time = convert_time(tm);
	s64 ts = steps_between(profile_base_time, time, parent->app->time_step_size);
	if(ts < 0 && ts >= main_plot.profile.GetCount()) return;
	time_step_slider.SetData(ts);
	main_plot.setup.profile_time_step = ts;
	main_plot.replot_profile();
}

void PlotCtrl::play_pushed() {
	if(is_playing) {
		is_playing = false;
		KillTimeCallback();
		push_play.SetImage(IconImg9::Play());
	} else {
		is_playing = true;
		double total_duration = 4*1000; // 4 second playback time TODO: could be a setting
		double step_count = (double)time_step_slider.GetMax();
		int tick_ms = 40;
		int steps_per_tick = (int)((tick_ms * step_count) / total_duration);
		steps_per_tick = std::max(1, steps_per_tick);
		
		SetTimeCallback(-tick_ms, [this, steps_per_tick]() {
			int current = time_step_slider.GetData();
			int slider_max = time_step_slider.GetMax();
			int next = std::min(slider_max, current+steps_per_tick);
			time_step_slider.SetData(next);
			time_step_slider_event();
			if(next == slider_max) {
				is_playing = false;
				KillTimeCallback();
				push_play.SetImage(IconImg9::Play());
			}
		});
		
		push_play.SetImage(IconImg9::Pause());
	}
}

void PlotCtrl::plot_change() {
	if(!parent->model_is_loaded()) return;
	
	if(is_playing) {
		is_playing = false;
		KillTimeCallback();
		push_play.SetImage(IconImg9::Play());
	}
	
	get_plot_setup(main_plot.setup);
	
	Plot_Mode mode = main_plot.setup.mode;
	
	scatter_inputs.Disable();
	y_axis_mode.Disable();
	time_intervals.Disable();
	time_step_edit.Hide();
	time_step_slider.Hide();
	time_step_slider.Disable();
	push_play.Hide();
	push_rewind.Hide();
	pivot_month.Disable();
	
	if (mode == Plot_Mode::regular || mode == Plot_Mode::stacked || mode == Plot_Mode::stacked_share || mode == Plot_Mode::compare_baseline) {
		scatter_inputs.Enable();
		y_axis_mode.Enable();
		time_intervals.Enable();
	} else if (mode == Plot_Mode::residuals) {
		scatter_inputs.Enable();
		time_intervals.Enable();
	} else if (mode == Plot_Mode::profile) {
		time_step_slider.Show();
		time_step_edit.Show();
		push_play.Show();
		push_rewind.Show();
		time_intervals.Enable();
	} else if (mode == Plot_Mode::profile2D)
		time_intervals.Enable();
	
	if(time_intervals.IsEnabled()) {
		if(main_plot.setup.aggregation_period == Aggregation_Period::none)
			aggregation.Disable();
		else {
			aggregation.Enable();
			y_axis_mode.Disable();
			time_step_edit.Disable();
		}
		if(main_plot.setup.aggregation_period == Aggregation_Period::yearly)
			pivot_month.Enable();
	} else
		aggregation.Disable();
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		bool active = main_plot.setup.index_set_is_active[idx];
		index_list[idx]->Enable(active);
		push_sel_all[idx]->Enable(active);
	}
	
	main_plot.build_plot();
}

void PlotCtrl::re_plot(bool caused_by_run) {
	get_plot_setup(main_plot.setup);
	main_plot.build_plot(caused_by_run);
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(index_list[idx]->GetSelectCount() != index_list[idx]->GetCount())
			push_sel_all[idx]->Set(false);
	}
}

void PlotCtrl::build_time_intervals_ctrl() {
	
	time_intervals.Reset();
	time_intervals.Add((int)Aggregation_Period::none, "No aggr.");
	time_intervals.SetData((int)Aggregation_Period::none);
	
	if(!parent->model_is_loaded()) return;
	
	auto ts = parent->app->time_step_size;
	
	if(ts.unit == Time_Step_Size::second) {
		if(ts.multiplier < 7*86400)
			time_intervals.Add((int)Aggregation_Period::weekly, "Weekly");
		if(ts.multiplier < 30*86400)
			time_intervals.Add((int)Aggregation_Period::monthly, "Monthly");
		if(ts.multiplier < 365*86400)
			time_intervals.Add((int)Aggregation_Period::yearly, "Yearly");
	} else if(ts.unit == Time_Step_Size::month) {
		if(ts.multiplier < 12)
			time_intervals.Add((int)Aggregation_Period::yearly, "Yearly");
	} else
		parent->log("Unhandled time step unit type.", true);
}

void PlotCtrl::clean() {
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_list[idx]->Clear();
		index_list[idx]->Hide();
		push_sel_all[idx]->Disable();
		push_sel_all[idx]->Hide();
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
	for(auto index_set_id : model->index_sets) {
		auto index_set = model->index_sets[index_set_id];
		
		index_list[idx]->HeaderTab(0).SetText(index_set->name.data());
		
		// NOTE: For now, display the maximum number of index sets for this index.
		// TODO: Could make it somehow dynamic based on other selections, but it is tricky.
		for(Index_T index = {index_set_id, 0}; index < app->index_data.get_max_count(index_set_id); ++index)
			index_list[idx]->Add(app->index_data.get_index_name_base(index, invalid_index)); // TODO: Don't use get_index_name_base
		
		index_list[idx]->GoBegin();
		index_list[idx]->Show();
		push_sel_all[idx]->Show();
		
		index_sets[idx] = index_set_id;
		
		++idx;
		
		if(idx == MAX_INDEX_SETS) {
			parent->log("The model has more index sets than are supported by MobiView2!", true);
			break;
		}
	}
	
	plot_major_mode.Enable();
}

void
register_if_index_set_is_active(Plot_Setup &ps, Model_Application *app) {
	
	for(Var_Id var_id : ps.selected_series) {
		const std::vector<Entity_Id> &var_index_sets = var_id.type == Var_Id::Type::series ? app->series_structure.get_index_sets(var_id) : app->additional_series_structure.get_index_sets(var_id);
		int idx = 0;
		for(auto index_set : app->model->index_sets) {
			if(std::find(var_index_sets.begin(), var_index_sets.end(), index_set) != var_index_sets.end())
				ps.index_set_is_active[idx] = true;
			++idx;
		}
	}
	for(Var_Id var_id : ps.selected_results) {
		const std::vector<Entity_Id> &var_index_sets = app->result_structure.get_index_sets(var_id);
		int idx = 0;
		for(auto index_set : app->model->index_sets) {
			if(std::find(var_index_sets.begin(), var_index_sets.end(), index_set) != var_index_sets.end())
				ps.index_set_is_active[idx] = true;
			++idx;
		}
	}
}
	
void
PlotCtrl::get_plot_setup(Plot_Setup &ps) {
	ps.selected_results.clear();
	ps.selected_series.clear();
	ps.selected_indexes.clear();
	ps.selected_indexes.resize(MAX_INDEX_SETS);
	ps.index_set_is_active.clear();
	ps.index_set_is_active.resize(MAX_INDEX_SETS, false);
	
	//if(time_intervals.IsEnabled()) {
		ps.aggregation_period = (Aggregation_Period)(int)time_intervals.GetData();
		ps.aggregation_type   = (Aggregation_Type)(int)aggregation.GetData();
	//} else
	//	ps.aggregation_period = Aggregation_Period::none;
	
	ps.pivot_month = pivot_month.GetData();
	
	ps.mode = (Plot_Mode)(int)plot_major_mode.GetData();
	
	//if(y_axis_mode.IsEnabled())
		ps.y_axis_mode = (Y_Axis_Mode)(int)y_axis_mode.GetData();
	//else
	//	ps.y_axis_mode = Y_Axis_Mode::regular;
	
	//ps.scatter_inputs = (scatter_inputs.IsEnabled() && (bool)scatter_inputs.Get());
	ps.scatter_inputs = (bool)scatter_inputs.Get();
	
	if(!parent->model_is_loaded()) return;
	
	parent->input_selecter.get_selected(ps.selected_series);
	parent->result_selecter.get_selected(ps.selected_results);
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(!index_list[idx]->IsVisible()) continue;
		int row_count = index_list[idx]->GetCount();
		for(int row = 0; row < row_count; ++row) {
			if(!index_list[idx]->IsSelected(row)) continue;
			ps.selected_indexes[idx].push_back(Index_T { index_sets[idx], row });
		}
	}
	
	register_if_index_set_is_active(ps, parent->app);
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
	if(ps.selected_indexes.empty()) return;   // Means that it was not properly initialized. Can happen when setting it from additional plot view.
	
	plot_major_mode.SetData((int)ps.mode);
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
	if(ps.pivot_month >= 1 && ps.pivot_month <= 12)
		pivot_month.SetData(ps.pivot_month);
	
	parent->result_selecter.set_selection(ps.selected_results);
	parent->input_selecter.set_selection(ps.selected_series);
	
	plot_change();
}