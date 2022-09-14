#include "PlotCtrl.h"
#include "MobiView2.h"

using namespace Upp;


MyPlot::MyPlot() {
	//this->SetFastViewX(true); Can't be used with scatter plot data since it combines points.
	SetSequentialXAll(true);
	
	Size plot_reticle_size = GetTextSize("00000000", GetReticleFont());
	Size plot_unit_size    = GetTextSize("[dummy]", GetLabelsFont());
	SetPlotAreaLeftMargin(plot_reticle_size.cx + plot_unit_size.cy + 20);
	SetPlotAreaBottomMargin(plot_reticle_size.cy*2 + plot_unit_size.cy + 20);
	
	SetGridDash("");
	Color grey(180, 180, 180);
	SetGridColor(grey);
	
	RemoveMouseBehavior(ScatterCtrl::ZOOM_WINDOW);
	RemoveMouseBehavior(ScatterCtrl::SHOW_INFO);
	AddMouseBehavior(true, false, false, true, false, 0, false, ScatterCtrl::SHOW_INFO);
	AddMouseBehavior(false, false, false, true, false, 0, false, ScatterCtrl::SCROLL);
	AddMouseBehavior(false, false, false, false, true, 0, false, ScatterCtrl::SCROLL);
}

void MyPlot::clean(bool full_clean) {
	//TODO
	RemoveAllSeries();
	//plot_colors.reset();
	//plot_data.clear();
	
	series_data.Clear();
	x_data.clear();
	
	RemoveSurf();
	//surf_x.Clear();
	//surf_y.Clear();
	//surf_z.Clear();
	
	SetTitle("");
	SetLabelX(" ");
	SetLabelY(" ");
	//cached_stack_y.clear();
	
	if(full_clean)
		was_auto_resized = false;
}

void MyPlot::compute_x_data(Date_Time start, s64 steps, Time_Step_Size ts_size) {
	x_data.resize(steps);
	Expanded_Date_Time dt(start, ts_size);
	for(s64 step = 0; step < steps; ++step) {
		x_data[step] = (double)dt.date_time.seconds_since_epoch;
		dt.advance();
	}
}

void MyPlot::build_plot(bool cause_by_run, Plot_Mode override_mode) {
	
	clean(false);
	
	if(!parent->model_is_loaded()) {
		SetTitle("No model is loaded.");
		return;
	}
	
	int series_count = setup.selected_results.size() + setup.selected_series.size();
	if(series_count == 0) {
		SetTitle("No time series is selected for plotting.");
		return;
	}
	
	auto app = parent->app;
	
	Date_Time result_start = app->result_data.start_date;
	Date_Time input_start  = app->series_data.start_date;  // NOTE: Should be same for both type of series (model inputs & additional series)
	
	s64 input_ts = app->series_data.time_steps;
	s64 result_ts = app->result_data.time_steps;
	if(input_ts == 0) {   //NOTE: could happen if there are no input series at all
		input_ts = result_ts;
		input_start = result_start;
	}
	s64 result_offset = steps_between(input_start, result_start, app->time_step_size);
	
	
	if(!setup.selected_results.empty() && result_ts == 0) {
		SetTitle("Unable to generate a plot of any result series since the model has not been run.");
		return;
	}
	
	// NOTE: this should theoretically never happen
	if(setup.selected_indexes.empty()) {
		SetTitle("No indexes were selected");
		return;
	}
	
	bool multi_index = false;
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(setup.selected_indexes[idx].empty() && setup.index_set_is_active[idx]) {
			SetTitle("At least one index has to be selected for each of the active index sets.");
			return;
		}
		if(setup.selected_indexes[idx].size() > 1 && setup.index_set_is_active[idx])
			multi_index = true;
	}
	
	ShowLegend(true);
	
	plot_info->Clear();
	
	Plot_Mode mode = setup.mode;
	if(override_mode != Plot_Mode::none) mode = override_mode;
	
	int n_bins_histogram = 0;
	
	bool residuals_available = false;
	bool compute_gof = (bool)parent->gof_option.GetData() || mode == Plot_Mode::residuals || mode == Plot_Mode::residuals_histogram || mode == Plot_Mode::qq;
	
	Date_Time gof_start;
	Date_Time gof_end;
	s64 input_gof_offset;
	s64 result_gof_offset;
	s64 gof_ts;
	
	{
		//TODO: We could maybe factor out some stuff here!
		Time gof_start_setting  = parent->calib_start.GetData();
		Time gof_end_setting    = parent->calib_end.GetData();
		
		Date_Time result_end = advance(result_start, app->time_step_size, result_ts-1);
		gof_start = IsNull(gof_start_setting) ? result_start : convert_time(gof_start_setting);
		gof_end   = IsNull(gof_end_setting)   ? result_end   : convert_time(gof_end_setting);
		
		if(gof_start < result_start || gof_start > result_end)
			gof_start = result_start;
		if(gof_end   < result_start || gof_end   > result_end || gof_end < gof_start)
			gof_end   = result_end;
		
		gof_ts = steps_between(gof_start, gof_end, app->time_step_size) + 1; //NOTE: if start time = end time, there is still one timestep.
		result_gof_offset = steps_between(result_start, gof_start, app->time_step_size);
		if(mode == Plot_Mode::residuals || mode == Plot_Mode::residuals_histogram || mode == Plot_Mode::qq)
			input_gof_offset = result_gof_offset;
		else
			input_gof_offset = steps_between(input_start, gof_start, app->time_step_size);
	}
	
	compute_x_data(input_start, input_ts, app->time_step_size);
	
	
	//String sim_legend, obs_legend, sim_unit, obs_unit;
	//TODO: compute residual stats
	
	
	
	if(mode == Plot_Mode::regular) {
		//NOTE: test code.
		if(setup.selected_results.size() > 0) {
			std::vector<Index_T> indexes(MAX_INDEX_SETS);
			for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
				if(setup.selected_indexes[idx].size() > 0)
					indexes[idx] = setup.selected_indexes[idx][0];
				else
					indexes[idx] = invalid_index;
			}
			Var_Id var_id = setup.selected_results[0];
			s64 offset = app->result_data.get_offset(var_id, indexes);
			
			series_data.Create(app, 0, offset, result_offset, 0, result_ts, x_data.data());
			
			String name = str(app->model->state_vars[var_id]->name);
			Color graph_color(255, 0, 0);
			AddSeries(series_data.Top()).NoMark().Stroke(1.5, graph_color).Dash("").Legend(name);
			
			//PromptOK(Format("Offset %d result_offset %d result_ts %d x_data_size %d", offset, result_offset, result_ts, (int)x_data.size()));
		}
		
	} else {
		SetTitle("Plot mode not yet implemented :(");
		return;
	}
	
	//TODO!
	ZoomToFit(true, true);
	
	//TODO!
}


double Mobius_Data_Source::y(s64 id) {
	s64 ts = id + y_offset;
	if(type == 0)
		return *app->result_data.get_value(offset, ts);
	else if(type == 1)
		return *app->series_data.get_value(offset, ts);
	else if(type == 2)
		return *app->additional_series_data.get_value(offset, ts);
}