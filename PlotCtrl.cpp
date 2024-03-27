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
	
	scatter_inputs.SetData(true);
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

void
PlotCtrl::build_index_set_selecters(Model_Application *app) {
	if(!app) return;
	auto model = app->model;
	
	Indexes indexes(model);
	
	for(auto index_set_id : model->index_sets) {
		auto index_set = model->index_sets[index_set_id];
		
		auto &index_list = index_lists.Create<ArrayCtrl>();
		index_list.AddColumn(index_set->name.data());
		index_list.MultiSelect();
		index_list.Hide();
		index_list.WhenSel << [this, index_set_id](){ index_selection_change(index_set_id, true); };
		
		auto &push_sel_all = push_sel_alls.Create<ButtonOption>();
		push_sel_all.Hide();
		push_sel_all.SetImage(IconImg9::Add(), IconImg9::Remove());
		push_sel_all.WhenAction << [this, index_set_id]() {
			
			index_callback_lock = true;
			
			auto &list = index_lists[index_set_id.id];
			int rows = list.GetCount();
			int count = list.GetSelectCount();
			if(rows == count) {
				list.Select(0, rows, false);
				list.Select(0);
			} else
				list.Select(0, rows);
			index_callback_lock = false;
			
			index_selection_change(index_set_id, true);
		};
		
		index_list.BottomPosZ(28, 104);
		push_sel_all.BottomPosZ(4, 20);
		
		Add(index_list);
		Add(push_sel_all);
		
		auto count = app->index_data.get_index_count(indexes, index_set_id);
		for(Index_T index = {index_set_id, 0}; index < count; ++index)
			index_list.Add(app->index_data.get_index_name(indexes, index));
		
		indexes.set_index(Index_T {index_set_id, 0 });
	}
	
	index_callback_lock = true;
	for(auto &list : index_lists)
		list.GoBegin();
	index_callback_lock = false;
	
	plot_major_mode.Enable();
}

void
PlotCtrl::index_selection_change(Entity_Id id, bool replot) {
	if(index_callback_lock) return;
	
	// TODO: This becomes a bit bonkers if we could have multiple active parent sets of the
	// same index sets. This happens if we select two series and a third set is sub-indexed to
	// a union of sets that the two series depend on.
	// In this case we should probably show the union only and not the two separate members,
	// but it is tricky, because we would then have to re-direct the plot itself to only look
	// up the union (if it is active).
	
	get_plot_setup(main_plot.setup);
	
	if(main_plot.setup.selected_indexes[id.id].empty()) return;
	
	Indexes indexes(parent->model);
	indexes.set_index(main_plot.setup.selected_indexes[id.id][0]);
	
	index_callback_lock = true;
	
	for(auto other_id : parent->model->index_sets) {
		if(id == other_id) continue;
		bool active = main_plot.setup.index_set_is_active[id.id];
		if(!active) continue;
		
		// Re-build the index list of a sub-indexed index set.
		
		if(!parent->app->index_data.can_be_sub_indexed_to(id, other_id)) continue;
		
		auto count = parent->app->index_data.get_index_count(indexes, other_id);
		
		auto &list = index_lists[other_id.id];
		list.ClearSelection();
		list.Clear();
		
		for(Index_T index = {other_id, 0}; index < count; ++index)
			list.Add(parent->app->index_data.get_index_name(indexes, index));
		
		int c = list.GetCount();
		for(auto index : main_plot.setup.selected_indexes[other_id.id]) {
			if(index.index < c)
				list.Select(index.index);
		}
	}
	
	get_plot_setup(main_plot.setup); // Have to do this to make the setup reflect the new state in case it changed.
	
	index_callback_lock = false;
	
	if(replot)
		re_plot(false);
}

void
PlotCtrl::plot_change() {
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
	
	// TODO: All this stuff is really only needed if the active index sets changed.
	int active_idx = 0;
	for(auto id : parent->model->index_sets) {
		bool active = main_plot.setup.index_set_is_active[id.id];
		auto index_set = parent->model->index_sets[id];
		
		index_lists[id.id].Show(active);
		push_sel_alls[id.id].Show(active);
		
		if(!active) continue;
		
		if(active_idx == MAX_INDEX_SETS)
			parent->log("With this many index sets active in the plot, the selecters may not display correctly.", true);
		
		auto &list = index_lists[id.id];
		auto &push = push_sel_alls[id.id];
		
		list.LeftPosZ(4 + 92*active_idx, 88);
		push.LeftPosZ(4 + 92*active_idx, 20);
		
		list.MultiSelect();
		push.Enable();
		for(auto other_id : parent->model->index_sets) {
			if(id == other_id) continue;
			if(!main_plot.setup.index_set_is_active[other_id.id]) continue;
			
			if(!parent->app->index_data.can_be_sub_indexed_to(id, other_id)) continue;
				
			index_callback_lock = true;
			
			list.ClearSelection();
			list.MultiSelect(false);
			push.Disable();
			
			// When we turn off multiselect, we could only keep at most one index
			// selected
			if(main_plot.setup.selected_indexes[id.id].size() > 1) {
				int row = main_plot.setup.selected_indexes[id.id][0].index;
				list.Select(row);
				list.SetCursor(row);
			}
			
			index_callback_lock = false;
			
			// TODO: It should not always be necessary to call this:
			index_selection_change(id, false);
				
			break;
		}
		
		++active_idx;
	}
	
	main_plot.build_plot();
}

void PlotCtrl::re_plot(bool caused_by_run) {
	
	if(!parent->model_is_loaded()) return;
	
	get_plot_setup(main_plot.setup); // TODO: Is this needed?
	main_plot.build_plot(caused_by_run);
	
	for(auto index_set_id : parent->model->index_sets) {
		if(index_lists[index_set_id.id].GetSelectCount() != index_lists[index_set_id.id].GetCount())
			push_sel_alls[index_set_id.id].Set(false);
	}
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
	main_plot.clean();
	
	index_lists.Clear();
	push_sel_alls.Clear();
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
	
	for(auto id : app->model->index_sets) {
		// If this is a union index set and exactly one of the union members are active, we
		// should only activate the member, not the union.
		bool active = ps.index_set_is_active[id.id];
		auto index_set = app->model->index_sets[id];
		if(active && !index_set->union_of.empty()) {
			int count = 0;
			for(auto other_id : index_set->union_of) {
				if(ps.index_set_is_active[other_id.id])
					count++;
			}
			if(count == 1)
				ps.index_set_is_active[id.id] = false;
		}
	}
}
	
void
PlotCtrl::get_plot_setup(Plot_Setup &ps) {
	
	auto count = parent->model->index_sets.count();
	ps.selected_results.clear();
	ps.selected_series.clear();
	ps.selected_indexes.clear();
	ps.selected_indexes.resize(count);
	ps.index_set_is_active.clear();
	ps.index_set_is_active.resize(count, false);
	
	ps.aggregation_period = (Aggregation_Period)(int)time_intervals.GetData();
	ps.aggregation_type   = (Aggregation_Type)(int)aggregation.GetData();
	
	ps.pivot_month = pivot_month.GetData();
	
	ps.mode = (Plot_Mode)(int)plot_major_mode.GetData();
	
	ps.y_axis_mode = (Y_Axis_Mode)(int)y_axis_mode.GetData();
	
	ps.scatter_inputs = (bool)scatter_inputs.Get();
	
	if(!parent->model_is_loaded()) return;
	
	parent->input_selecter.get_selected(ps.selected_series);
	//TODO: This is just a quick fix due to how quick_select now works. It is not that elegant.
	std::vector<Var_Id> sel;
	parent->result_selecter.get_selected(sel);
	for(auto var_id : sel) {
		if(var_id.type == Var_Id::Type::state_var)
			ps.selected_results.push_back(var_id);
		else if(var_id.type == Var_Id::Type::series || var_id.type == Var_Id::Type::additional_series) {
			if(std::find(ps.selected_series.begin(), ps.selected_series.end(), var_id) == ps.selected_series.end())
				ps.selected_series.push_back(var_id);
		}
	}
	
	for(auto id : parent->model->index_sets) {
		auto &list = index_lists[id.id];

		int row_count = list.GetCount();
		int cursor = list.GetCursor();
		for(int row = 0; row < row_count; ++row) {
			bool selected = list.IsSelected(row);
			if(!list.IsMultiSelect()) selected = (row == cursor); // TODO: Not sure why IsSelected doesn't work in this case.
			if(selected)
				ps.selected_indexes[id.id].push_back(Index_T { id, row });
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
	
	for(auto id : parent->model->index_sets) {
		index_lists[id.id].ClearSelection(false);
		int row_count = index_lists[id.id].GetCount();
		/*
		for(int row = 0; row < row_count; ++row) {
			Index_T index = { id, row };
			if(std::find(ps.selected_indexes[id.id].begin(), ps.selected_indexes[id.id].end(), index) != ps.selected_indexes[id.id].end())
				index_list[id.id]->Select(row);
		}
		*/
		for(auto index : ps.selected_indexes[id.id])
			index_lists[id.id].Select(index.index);
	}
	if(ps.pivot_month >= 1 && ps.pivot_month <= 12)
		pivot_month.SetData(ps.pivot_month);
	
	parent->result_selecter.set_selection(ps.selected_results);
	parent->input_selecter.set_selection(ps.selected_series);
	
	plot_change();
}



double
Agg_Data_Source::y(s64 id) {
	double yval = get_actual_y(id);
	
	if(y_axis_mode == Y_Axis_Mode::normalized)
		return yval / max;
	//else if(y_axis_mode == Y_Axis_Mode::logarithmic)   // This is now automatic.
	//	return std::log10(yval);
	
	return yval;
}