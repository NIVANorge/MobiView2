#include "SensitivityViewWindow.h"
#include "MobiView2.h"

// NOTE: For the run icon
#define IMAGECLASS IconImg3
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

StatPlotCtrl::StatPlotCtrl() {
	CtrlLayout(*this);
}

SensitivityViewWindow::SensitivityViewWindow(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "MobiView2 perturbation view");
	Sizeable().Zoomable();
	
	plot.parent = parent;
	
	edit_steps.Min(1);
	edit_steps.SetData(10);
	
	push_run.SetImage(IconImg3::Run());
	push_run.WhenPush = THISBACK(run);
	
	run_progress.Hide();
	stat_plot.select_stat.Add(-1, "(none)");
	#define SET_STATISTIC(handle, name, type) stat_plot.select_stat.Add((int)Stat_Type::handle, name);
	#include "support/stat_types.incl"
	#undef SET_STATISTIC
	#define SET_RESIDUAL(handle, name, type) stat_plot.select_stat.Add((int)Residual_Type::handle, name);
	#include "support/residual_types.incl"
	#undef SET_RESIDUAL
	
	stat_plot.select_stat.GoBegin();
	
	main_horizontal.Horz();
	main_horizontal.Add(plot);
	main_horizontal.Add(stat_plot);
	main_horizontal.SetPos(7500, 0);
	
	Add(main_horizontal.VSizePos(52, 32).HSizePos());
	
	par.id = invalid_entity_id;
	
	stat_plot.plot.ShowLegend(false);
	stat_plot.plot.SetMouseHandling(false, false);
	// a bit annoying to repeat this for the stat plot..
	Size plot_reticle_size = GetTextSize("00000", stat_plot.plot.GetReticleFont());
	Size plot_unit_size    = GetTextSize("[dummy]", stat_plot.plot.GetLabelsFont());
	stat_plot.plot.SetPlotAreaLeftMargin(plot_reticle_size.cx + plot_unit_size.cy + 20);
	stat_plot.plot.SetPlotAreaBottomMargin(plot_reticle_size.cy + plot_unit_size.cy + 20);
	
	auto min_max_change = [this]() {
		if(is_valid(par.id)) {
			double min = edit_min.GetData();
			double max = edit_max.GetData();
			cached_min_max[par.id] = {min, max};
		}
	};
	edit_min.WhenAction << min_max_change;
	edit_max.WhenAction << min_max_change;
}

void
SensitivityViewWindow::update() {
	
	error_label.SetText("");
	param_label.SetText("");
	
	auto prev_id = par.id;
	par = parent->params.get_selected_parameter();
	
	if(!is_valid(par.id) || !parent->model_is_loaded()) {
		error_label.SetText("Select a parameter in the main parameter view.");
		edit_min.SetData(Null);
		edit_max.SetData(Null);
		return;
	}
	
	auto par_data = parent->app->model->parameters[par.id];
	
	if(par_data->decl_type != Decl_Type::par_real) {
		//TODO: Allow par_int too?
		error_label.SetText("Can only perturb parameters of type real.");
		edit_min.SetData(Null);
		edit_max.SetData(Null);
		return;
	}
	
	String unit = "";
	if(is_valid(par_data->unit))
		unit = parent->app->model->units[par_data->unit]->data.to_utf8();

	String index_str = make_parameter_index_string(&parent->app->parameter_structure, &par);
	String label_text = Format("%s [%s] %s", par_data->name.data(), unit, index_str);
	
	param_label.SetText(label_text);
	if(prev_id != par.id) {
		double min, max;
		
		auto find = cached_min_max.find(par.id);
		if(find == cached_min_max.end()) {
			min = par_data->min_val.val_real;
			max = par_data->max_val.val_real;
			cached_min_max[par.id] = {min, max};
		} else {
			min = find->second.first;
			max = find->second.second;
		}
		edit_min.SetData(min);
		edit_max.SetData(max);
	}
}

void
SensitivityViewWindow::run() {
	double min = edit_min.GetData();
	double max = edit_max.GetData();
	
	error_label.SetText("");

	if(!is_valid(par.id)) {
		error_label.SetText("Select a parameter in the main view");
		return;
	}
	
	if(IsNull(min) || IsNull(max)) {
		error_label.SetText("Min and Max must be real values");
		return;
	}
	
	if(max <= min) {
		error_label.SetText("Max must be larger than Min");
		return;
	}
	
	parent->plotter.get_plot_setup(plot.setup);
	if(plot.setup.selected_results.size() != 1 || plot.setup.selected_series.size() > 1) {
		error_label.SetText("This currently only works with a single selected result series, and at most one input series");
		return;
	}
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(plot.setup.selected_indexes[idx].size() != 1 && plot.setup.index_set_is_active[idx]) {
			error_label.SetText("This currently only works with a single selected index per index set for the time series");
			return;
		}
	}
	
	plot.clean(false);
	
	bool has_input = plot.setup.selected_series.size() == 1;
	
	if(plot.setup.y_axis_mode == Y_Axis_Mode::normalized)
		plot.setup.y_axis_mode = Y_Axis_Mode::regular;
	plot.setup.mode = Plot_Mode::regular;
	
	int n_runs = (int)edit_steps.GetData() + 1; //NOTE: it is more intuitive that e.g. 10 steps from 0 to 10 gives a stride of 1.
	
	double stride = (max - min) / (double)(n_runs - 1);
	
	//TODO: don't copy results (when we have that functionality)
	Model_Data *model_data = parent->app->data.copy();
	
	Date_Time input_start  = model_data->series.start_date;
	s64 input_ts           = model_data->series.time_steps;
	
	Date_Time result_start = model_data->get_start_date_parameter();
	s64 result_ts          = steps_between(result_start, model_data->get_end_date_parameter(), parent->app->time_step_size) + 1;
	
	// NOTE: it is a bit annoying that we have to do this: Maybe it should be functionality on
	// the Model_Application instead
	if(input_ts == 0) {
		input_start = result_start;
		input_ts    = result_ts;
	}
	
	Date_Time gof_start;
	Date_Time gof_end;
	s64 input_gof_offset;
	s64 result_gof_offset;
	s64 gof_ts;
	
	Time gof_start_setting  = parent->calib_start.GetData();
	Time gof_end_setting    = parent->calib_end.GetData();
	get_gof_offsets(gof_start_setting, gof_end_setting, input_start, input_ts, result_start, result_ts, gof_start, gof_end,
		input_gof_offset, result_gof_offset, gof_ts, parent->app->time_step_size, true);
	
	run_progress.Set(0);
	run_progress.SetTotal(n_runs);
	run_progress.Show();
	ProcessEvents();
	
	int GUI_update_freq = std::max(1, std::min(10, n_runs / 10));
	
	stat_plot.plot.RemoveAllSeries();
	stat_plot.plot.SetTitle(" ");
	stat_plot.plot.SetLabelX(" ");
	stat_plot.plot.SetLabelY(" ");
	
	std::vector<Index_T> indexes;
	get_single_indexes(indexes, plot.setup);
	
	Var_Id var_id = plot.setup.selected_results[0];
	
	auto *data = &model_data->get_storage(var_id.type);
	auto var = parent->app->vars[var_id];
	
	
	String result_unit = var->unit.to_utf8();
	s64 offset = data->structure->get_offset(var_id, indexes);
	s64 offset_ser = -1;
	
	String index_str = make_index_string(&parent->app->result_structure, indexes, var_id);
	plot.SetTitle(Format("Sensitivity of \"%s\" [%s] %s to %s",
		var->name.data(),
		result_unit,
		index_str,
		param_label.GetText()));
	
	std::vector<double> stat_data(n_runs), par_values(n_runs);
	for(int run = 0; run < n_runs; ++run) {
		stat_data[run] = std::numeric_limits<double>::quiet_NaN();
		par_values[run] = min + ((double)run)*stride;   //TODO: Also allow selection of logarithmic spacing
	}

	plot.compute_x_data(input_start, input_ts, parent->app->time_step_size);
	
	Data_Storage<double, Var_Id> *data_ser;
	State_Var *var_ser;
	Var_Id ser_id;
	if(has_input) {
		ser_id = plot.setup.selected_series[0];
		data_ser = &model_data->get_storage(ser_id.type);
		var_ser  = parent->app->vars[ser_id];
		offset_ser = data_ser->structure->get_offset(ser_id, indexes);
	}
	
	String stat_name = stat_plot.select_stat.Get();
	bool has_stat = (stat_name != "(none)");
	
	Data_Source_Owns_XY *stat_series = nullptr;
	if(has_stat) {
		plot.series_data.Create<Data_Source_Owns_XY>(&par_values, &stat_data);
		stat_series = reinterpret_cast<Data_Source_Owns_XY *>(&plot.series_data.Top());
		
		Color stat_color(0, 130, 200);
		stat_plot.plot.AddSeries(*stat_series).MarkBorderColor(stat_color).MarkColor(stat_color).Stroke(1.5, stat_color).Opacity(0.5).MarkStyle<CircleMarkPlot>();
		stat_plot.plot.SetLabels(param_label.GetText(), stat_name);
		stat_plot.plot.SetXYMin(min);
		stat_plot.plot.SetRange(max-min);
		stat_plot.plot.SetMajorUnitsNum(std::min(n_runs-1, 9));     //TODO: This should be better!
	} else
		stat_plot.plot.SetTitle("Select a statistic to display");
	
	Time_Series_Stats stats;
	Residual_Stats residual_stats;
	
	try {
		for(int run = 0; run < n_runs; ++run) {
			Parameter_Value par_val;
			par_val.val_real = min + ((double)run)*stride;
			
			set_parameter_value(par, model_data, par_val);
			run_model(model_data);

			Color color = GradientColor(Color(245, 130, 48), Color(145, 30, 180), run, n_runs);
			
			// NOTE: pass 0 to gof_offset and gof_ts here since it's plot_info is 0 any way.
			add_single_plot(&plot, model_data, parent->app, var_id, indexes, result_ts, input_start, result_start,
				plot.x_data.data(), 0, 0, color, false, Null, true);
			String legend;
			if(run == 0)
				legend = Format("%s = %g", param_label.GetText(), par_val.val_real);
			else
				legend = Format("%g", par_val.val_real);
			plot.Legend(legend); // Overwrite the legend
			
			//TODO: This is a bit inefficient.
			if(has_stat) {
				int type = stat_plot.select_stat.GetData();
				Stat_Class typetype = is_stat_class(type);
				
				if(typetype == Stat_Class::stat)
					compute_time_series_stats(&stats, &parent->stat_settings.settings, data, offset, result_gof_offset, gof_ts);
				else if(typetype == Stat_Class::residual)
					compute_residual_stats(&residual_stats, data, offset, result_gof_offset, data_ser, offset_ser, input_gof_offset, gof_ts, type==(int)Residual_Type::srcc);
				
				double val = get_stat(&stats, &residual_stats, type);
				stat_series->set_y(run) = val;
			}
			
			if(run % GUI_update_freq == 0 || run == n_runs-1){
				run_progress.Set(run+1);
				format_axes(&plot, Plot_Mode::regular, 0, input_start, parent->app->time_step_size);
				if(has_stat) {
					stat_plot.plot.ZoomToFit(true, true);
					set_round_grid_line_positions(&stat_plot.plot, 1);
				}
				ProcessEvents();
			}
		}
		
		// Add this at the end to not have it auto-zoom to input bounds. Fix zoom in other
		// ways...
		if(has_input) {
			Color ser_col(0, 130, 200);
			// NOTE: it is ok to pass 0 for gof_offset and gof_ts here since plot.plot_info=nullptr
			add_single_plot(&plot, model_data, parent->app, ser_id, indexes,
				input_ts, input_start, input_start, plot.x_data.data(), 0, 0, ser_col, false, Null, true);
		}
		
		// Have to do it at the end, since otherwise the plot min and max y are not set yet.
		// Could allow updating y values of these too, to have it in during run.
		if(has_stat) {
			double start_x = (double)(gof_start.seconds_since_epoch - input_start.seconds_since_epoch);
			double end_x   = (double)(gof_end.seconds_since_epoch   - input_start.seconds_since_epoch);
			add_line(&plot, start_x, plot.GetYMin(), start_x, plot.GetYMax(), Red());
			add_line(&plot, end_x, plot.GetYMin(), end_x, plot.GetYMax(), Red());
		}
	
	} catch(int) {
		error_label.SetText("An error occurred while running the model. See the main log box.");
	}
	parent->log_warnings_and_errors();
	run_progress.Hide();
	delete model_data;
}