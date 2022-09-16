#include "PlotCtrl.h"
#include "MobiView2.h"
#include "Statistics.h"

using namespace Upp;

std::vector<Color> Plot_Colors::colors = {{0, 130, 200}, {230, 25, 75}, {245, 130, 48}, {145, 30, 180}, {60, 180, 75},
                  {70, 240, 240}, {240, 50, 230}, {210, 245, 60}, {250, 190, 190}, {0, 128, 128}, {230, 190, 255},
                  {170, 110, 40}, {128, 0, 0}, {170, 255, 195}, {128, 128, 0}, {255, 215, 180}, {0, 0, 128}, {255, 225, 25}};


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
	colors.reset();
	
	series_data.Clear();
	x_data.clear();
	data_stacked.clear();
	
	RemoveSurf();
	//surf_x.Clear();
	//surf_y.Clear();
	//surf_z.Clear();
	
	SetTitle("");
	SetLabelX(" ");
	SetLabelY(" ");
	
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

void add_plot_recursive(MyPlot *draw, Model_Application *app, Var_Id var_id, std::vector<Index_T> &indexes, int level,
	s64 x_offset, s64 y_offset, s64 time_steps, double *x_data, const std::vector<Entity_Id> &index_sets) {
	if(level == draw->setup.selected_indexes.size()) {
		String legend;
		String unit = "";  //TODO!
		
		Structured_Storage<double, Var_Id> *data;
		if(var_id.type == 0) {
			data = &app->result_data;
			legend = str(app->model->state_vars[var_id]->name);
		} else if(var_id.type == 1) {
			data = &app->series_data;
			legend = str(app->model->series[var_id]->name);
		} else if(var_id.type == 2) {
			data = &app->additional_series_data;
			legend = str(app->additional_series[var_id]->name);
		}
		s64 offset = data->get_offset(var_id, indexes);
		//TODO: add indexes to name in legend
		
		draw->series_data.Create(data, offset, x_offset, y_offset, time_steps, x_data);
		
		//TODO: different formatting for series
		//TODO: aggregation etc.
		Color graph_color = draw->colors.next();
		if(var_id.type == 0 && (draw->setup.mode == Plot_Mode::stacked || draw->setup.mode == Plot_Mode::stacked_share)) {
			draw->data_stacked.Add(draw->series_data.Top());
			draw->AddSeries(draw->data_stacked.top()).Fill(graph_color);
		} else
			draw->AddSeries(draw->series_data.Top());
		if(var_id.type == 0 || !draw->setup.scatter_inputs)
			draw->NoMark().Stroke(1.5, graph_color).Dash("").Legend(legend).Units(unit);
		else {
			draw->MarkBorderColor(graph_color).Stroke(0.0, graph_color).Opacity(0.5).MarkStyle<CircleMarkPlot>();
			int index = draw->GetCount()-1;
			draw->SetMarkColor(index, Null); //NOTE: Calling draw->MarkColor(Null) does not make it transparent, so we have to do it like this.
		}
		
		Time_Series_Stats stats;
		compute_time_series_stats(&stats, &draw->parent->stat_settings.settings, data, offset, y_offset, time_steps);
		display_statistics(draw->plot_info, &draw->parent->stat_settings.display_settings, &stats, graph_color, legend, unit);
	} else {
		bool loop = false;
		if(!draw->setup.selected_indexes[level].empty()) {
			auto index_set = draw->setup.selected_indexes[level][0].index_set;
			auto find = std::find(index_sets.begin(), index_sets.end(), index_set);
			loop = find != index_sets.end();
		}
		if(!loop) {
			indexes[level] = invalid_index;
			add_plot_recursive(draw, app, var_id, indexes, level+1, x_offset, y_offset, time_steps, x_data, index_sets);
		} else {
			for(Index_T index : draw->setup.selected_indexes[level]) {
				indexes[level] = index;
				add_plot_recursive(draw, app, var_id, indexes, level+1, x_offset, y_offset, time_steps, x_data, index_sets);
			}
		}
	}
}

void set_round_grid_line_positions(MyPlot *plot, int axis) {
	//TODO: This should take Y axis mode into account (if axis is 1)
	
	double min;
	double range;
	
	if(axis == 0) {
		min   = plot->GetXMin();
		range = plot->GetXRange();
	} else {
		min   = plot->GetYMin();
		range = plot->GetYRange();
	}
	
	double log_range = std::log10(range);
	double int_part;
	double frac_part = std::modf(log_range, &int_part);
	if(range < 1.0) int_part -= 1.0;
	double order_of_mag = std::pow(10.0, int_part);
	double stretch = range / order_of_mag;
	constexpr int ref_ticks = 10; //TODO: Should depend on plot size.
	
	//TODO: Document this implementation better?
	static double stride_factors[8] = {0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0};
	double stride = order_of_mag * stride_factors[0];
	for(int idx = 1; idx < 8; ++idx) {
		double n_ticks = std::floor(range / stride);
		if((int)n_ticks <= ref_ticks) break;
		stride = order_of_mag * stride_factors[idx];
	}
	
	double min2 = std::floor(min / stride)*stride;
	range += min-min2;
	
	//MainPlot.SetMinUnits(Null, Min);  //We would prefer to do this, but for some reason it works
	//poorly when there are negative values...
	if(axis == 0) {
		plot->SetXYMin(min2, Null);
		plot->SetRange(range, Null);
		plot->SetMajorUnits(stride, Null);
	} else {
		plot->SetXYMin(Null, min2);
		plot->SetRange(Null, range);
		plot->SetMajorUnits(Null, stride);
	}
}

int round_step_10(int step) {
	//TODO: hmm, could this be unified with what is done in the above function somehow?
	int log_10 = (int)std::log10((double)step);
	int order_of_mag = (int)std::pow(10.0, log_10);
	//for(int idx = 0; idx < log_10; ++Idx) OrderOfMagnitude*=10;   //NOTE: Not efficient, but will not be a problem in practice
	int stretch = step / order_of_mag;
	if(stretch == 1) return order_of_mag;
	if(stretch == 2) return order_of_mag*2;
	if(stretch <= 5) return order_of_mag*5;
	return order_of_mag*10;
}

int round_step_60(int step) {
	//TODO: lookup table?
	if(step == 3) step = 2;
	else if(step == 4) step = 5;
	else if(step == 6 || step == 7) step = 5;
	else if(step >= 8 && step <= 12) step = 10;
	else if(step >= 13 && step <= 17) step = 15;
	else if(step >= 18 && step <= 26) step = 20;
	else if(step >= 27 && step <= 40) step = 30;
	return step;
}

int round_step_24(int step) {
	//TODO: lookup table?
	if(step == 3) step = 2;
	if(step == 5) step = 4;
	if(step == 7 || step == 8) step = 6;
	if(step >= 9 && step <= 14) step = 12;
	return step;
}

int round_step_31(int step) {
	// TODO: lookup table?
	if(step == 3) step = 2;
	else if(step == 4) step = 5;
	else if(step == 6 || step == 7) step = 5;
	else if(step == 8 || step == 9) step = 10;
	else if(step > 2 && step < 20) step = 15;
	return step;
}


void set_date_grid_line_positions_x(double x_min, double x_range, Vector<double> &pos, Date_Time input_start, int res_type) {
	constexpr int n_grid_lines = 10;  //TODO: Make it sensitive to plot size
	
	s64 sec_range = (s64)x_range;
	s64 first = input_start.seconds_since_epoch + (s64)x_min;
	s64 last  = first + sec_range;
	
	//NOTE: res_type denotes the unit that we try to use for spacing the grid lines. 0=seconds, 1=minutes, 2=hours, 3=days,
	//4=months, 5=years

	s64 step;
	s64 iter_time = first;

	if(res_type == 0) {
		step = sec_range / n_grid_lines + 1;
		step = round_step_60(step);
		
		if(step > 30) res_type = 1;    //The plot is too wide to do secondly resolution, try minutely instead
		else
			iter_time -= (iter_time % step); //TODO: may not work if iter_time is negative. Likewise for the next two. Fix this. (make round_down_by function.)
	}
	
	if(res_type == 1) {
		s64 min_range = sec_range / 60;
		s64 min_step = min_range / n_grid_lines + 1;
		min_step = round_step_60(min_step);
		
		if(min_step > 30) res_type = 2;    //The plot is too wide to do minutely resolution, try hourly instead
		else {
			iter_time -= (iter_time % 60);
			iter_time -= 60*((iter_time/60) % min_step);
			step = 60*min_step;
		}
	}
	
	if(res_type == 2) {
		s64 hr_range = sec_range / 3600;
		s64 hr_step  = hr_range / n_grid_lines + 1;
		hr_step = round_step_24(hr_step);
		
		if(hr_step > 12) res_type = 3;    //The plot is too wide to do hourly resolution, try daily instead
		else {
			iter_time -= (iter_time % 3600);
			iter_time -= 3600*((iter_time / 3600) % hr_step);
			step = 3600*hr_step;
		}
	}
	
	if(res_type <= 2) {
		if(step <= 0) //NOTE: should not happen. Just so that it doesn't crash if there is a bug.
			return;
		
		for(; iter_time <= last; iter_time += step) {
			double x = (double)((iter_time - input_start.seconds_since_epoch)) - x_min;
			if(x > 0.0 && x < x_range)
				pos << x;
		}
		return;
	}
	
	Date_Time first_d;
	first_d.seconds_since_epoch = first;
	Date_Time last_d;
	last_d.seconds_since_epoch = last;
	s32 fy, fm, fd, ly, lm, ld;
	first_d.year_month_day(&fy, &fm, &fd);
	last_d.year_month_day(&ly, &lm, &ld);
	
	if(res_type == 3) {
		s64 day_range = sec_range / 86400;   // +1 ??
		s64 day_step  = day_range / n_grid_lines + 1;
		day_step = round_step_31(day_step);

		if(day_step >= 20) res_type = 4; //The plot is too wide to do daily resolution, try monthly instead;
		else {
			s32 d = fd;
			d -= (d - 1) % day_step;
			s32 m = fm;
			s32 y = fy;
			
			// TODO: use Expanded_Date_Time here instead?
			while(true) {
				if(y > ly ||
					(y == ly && (m > lm ||
						(m == lm && d > ld)))) break;
				
				s32 dom = month_length(y, m);
				if(d > dom || (day_step != 1 && (dom - d < day_step/2))) {   //TODO: I forgot why rightmost part of the condition is here. Figure it out and explain it in comment.
					d = 1; m++;
					if(m > 12) { m = 1; y++; }
				}
				Date_Time iter_date(y, m, d);
				double x = (double)((iter_date.seconds_since_epoch - input_start.seconds_since_epoch)) - x_min;
				if(x > 0.0 && x < x_range)
					pos << x;
				
				d += day_step;
			}
			return;
		}
	}
	
	s64 mon_step;
	if(res_type == 4) {
		int mon_range = lm - fm + 12*(ly - fy);
		mon_step = mon_range / n_grid_lines + 1;
		
		if(mon_step >= 10) res_type = 5; //The plot is too wide to do monthly resolution, try yearly instead
		else {
			if(mon_step == 4) mon_step = 3;
			else if(mon_step == 5 || (mon_step > 6 && mon_step <= 9)) mon_step = 6;
			
			int surp = (fm-1) % mon_step;
			fm -= surp; //NOTE: This should not result in fm being negative.
		}
	}
	
	if(res_type == 5) {
		int yr_range = ly - fy;
		int yr_step = yr_range / n_grid_lines + 1;
		yr_step = round_step_10(yr_step);
		fy -= (fy % yr_step);
		fm = 1;
		mon_step = 12*yr_step;
	}
	
	if(mon_step <= 0)  //NOTE: should not happen. Just so that it doesn't crash if there is a bug.
		return;
	
	Expanded_Date_Time iter_date(Date_Time(fy, fm, 1), Time_Step_Size { Time_Step_Size::month, (s32)mon_step} );
	
	while(true) {
		if(iter_date.year > ly || (iter_date.year == ly && iter_date.month > lm)) break;
		
		double x = (double)((iter_date.date_time.seconds_since_epoch - input_start.seconds_since_epoch)) - x_min;
		if(x > 0.0 && x < x_range)
			pos << x;
		iter_date.advance();
	}
		
}

int compute_smallest_step_resolution(Aggregation_Period interval_type, Time_Step_Size ts_size) {
	//NOTE: To compute the unit that we try to use for spacing the grid lines. 0=seconds, 1=minutes, 2=hours, 3=days,
	//4=months, 5=years
	if(interval_type == Aggregation_Period::none) {
		//NOTE: The plot does not display aggregated data, so the unit of the grid line step should be
		//determined by the model step size.
		if(ts_size.unit == Time_Step_Size::second) {
			if(ts_size.magnitude < 60)         return 0;
			else if(ts_size.magnitude < 3600)  return 1;
			else if(ts_size.magnitude < 86400) return 2;
			else                               return 3;
		} else {
			if(ts_size.magnitude < 12)         return 4;
			else                               return 5;
		}
	}
	else if(interval_type == Aggregation_Period::weekly)  return 3;
	else if(interval_type == Aggregation_Period::monthly) return 4;
	else if(interval_type == Aggregation_Period::yearly)  return 5;
	return 0;
}

inline void time_stamp_format(int res_type, Date_Time ref_date, double seconds_since_ref, String &str) {
	static const char *month_names[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	//NOTE: Adding 0.5 helps a bit with avoiding flickering when panning, but is not perfect (TODO)
	Date_Time d2 = ref_date + (s64)(seconds_since_ref + 0.5);
	s32 h, m, s;
	if(res_type <= 2) {
		d2.hour_minute_second(&h, &m, &s);
		if(res_type == 0)
			str = Format("%02d:%02d:%02d", h, m, s);
		else
			str = Format("%02d:%02d", h, m);
	}
	if(res_type >= 3 || (h==0 && m==0 && s==0)) {
		s32 y, mn, d;
		d2.year_month_day(&y, &mn, &d);
		if(res_type <= 3) {
			if(res_type <= 2)
				str << "\n";
			str << Format("%d. ", d);
		}
		if(d == 1 || res_type >= 4) {
			if(res_type <= 4)
				str << month_names[mn-1];
			if(mn == 1 || res_type >= 5) {
				if(res_type <= 4)
					str << "\n";
				str << Format("%d", y);
			}
		}
	}
}

void format_axes(MyPlot *plot, Plot_Mode mode, int n_bins_histogram, Date_Time input_start, Time_Step_Size ts_size) {
	plot->SetGridLinesX.Clear();
	
	if(plot->GetCount() > 0) {// || (mode == Plot_Mode::profile2D && SurfZ.size() > 0)) {
		if(mode == Plot_Mode::histogram || mode == Plot_Mode::residuals_histogram) {
			// TODO: We may want to use the new histogram functionality in ScatterDraw instead
			/*
			plot->cbModifFormatXGridUnits.Clear();
			plot->cbModifFormatX.Clear();
			//NOTE: Histograms require different zooming.
			plot->ZoomToFit(true, true);
			plot->was_auto_resized = false;
			
			double x_range = plot->GetXRange();
			double x_min   = plot->GetXMin();
			
			//NOTE: The auto-resize cuts out half of each outer bar, so we fix that
			double strinde = x_range / (double)(n_bins_histogram-1);
			XMin   -= 0.5*Stride;
			XRange += Stride;
			this->SetXYMin(XMin);
			this->SetRange(XRange);
			
			int LineSkip = NBinsHistogram / 30 + 1;
			
			this->SetGridLinesX << [NBinsHistogram, Stride, LineSkip](Vector<double> &LinesOut)
			{
				double At = 0.0;//this->GetXMin();
				for(int Idx = 0; Idx < NBinsHistogram; ++Idx)
				{
					LinesOut << At;
					At += Stride * (double)LineSkip;
				}
			};
			*/
		} else if(mode == Plot_Mode::qq) {
			//TODO
			/*
			this->cbModifFormatXGridUnits.Clear();
			this->cbModifFormatX.Clear();
			//NOTE: Histograms require completely different zooming.
			this->ZoomToFit(true, true);
			PlotWasAutoResized = false;
			
			double YRange = this->GetYRange();
			double YMin   = this->GetYMin();
			double XRange = this->GetXRange();
			double XMin   = this->GetXMin();
			
			//NOTE: Make it so that the extremal points are not on the border
			double ExtendY = YRange * 0.1;
			YRange += 2.0 * ExtendY;
			YMin -= ExtendY;
			double ExtendX = XRange * 0.1;
			XRange += 2.0 * ExtendX;
			XMin -= ExtendX;
			
			this->SetRange(XRange, YRange);
			this->SetXYMin(XMin, YMin);
			*/
		} else if(mode == Plot_Mode::profile) {
			plot->was_auto_resized = false;
		} else {
			plot->cbModifFormatXGridUnits.Clear();
			plot->cbModifFormatX.Clear();
			plot->ZoomToFit(false, true);
			
			int res_type = compute_smallest_step_resolution(plot->setup.aggregation_period, ts_size);
			
			//PromptOK(Format("res_type = %d", res_type));
			
			// Set the minimum of the y range to be 0 unless the minimum is already below 0
			double y_range = plot->GetYRange();
			double y_min   = plot->GetYMin();
			if(y_min > 0.0) {
				double new_range = y_range + y_min;
				plot->SetRange(Null, new_range);
				plot->SetXYMin(Null, 0.0);
			}
			
			if(!plot->was_auto_resized) {
				plot->ZoomToFit(true, false);
				plot->was_auto_resized = true;
			}
			
			// TODO: This is still bug prone!!
			// Position of x grid lines
			
			plot->SetGridLinesX << [plot, input_start, res_type](Vector<double> &vec){
				double x_min = plot->GetXMin();
				double x_range = plot->GetXRange();
				set_date_grid_line_positions_x(x_min, x_range, vec, input_start, res_type);
			};
			
			
			// Format to be displayed for x values at grid lines and data view
			plot->cbModifFormatX << [res_type, input_start] (String &str, int i, double r) {
				time_stamp_format(res_type, input_start, r, str);
			};
		}
		
		// Extend the Y range a bit more to avoid the legend obscuring the plot in the most
		// common cases (and to have a nice margin).
		if(plot->GetShowLegend() && mode != Plot_Mode::profile)
			plot->SetRange(Null, plot->GetYRange() * 1.15);
		
		if(mode != Plot_Mode::profile2D) {
			plot->cbModifFormatY.Clear();
			plot->cbModifFormatYGridUnits.Clear();
			if(plot->setup.y_axis_mode == Y_Axis_Mode::logarithmic) {
				plot->cbModifFormatYGridUnits << [](String &s, int i, double d) {
					s = FormatDoubleExp(pow(10., d), 2);
				};
			} else {
				plot->cbModifFormatYGridUnits << [](String &s, int i, double d) {
					s = FormatDouble(d, 2);
				};
			}

			set_round_grid_line_positions(plot, 1);
		}

		if(mode == Plot_Mode::qq)
			set_round_grid_line_positions(plot, 0);
	}
	
	bool allow_scroll_x = !(mode == Plot_Mode::profile || mode == Plot_Mode::histogram || mode == Plot_Mode::residuals_histogram);
	bool allow_scroll_y = mode == Plot_Mode::qq;
	plot->SetMouseHandling(allow_scroll_x, allow_scroll_y);
}

void get_single_indexes(std::vector<Index_T> &indexes, Plot_Setup &setup) {
	indexes.resize(MAX_INDEX_SETS);
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(setup.selected_indexes[idx].empty())
			indexes[idx] = invalid_index;
		else
			indexes[idx] = setup.selected_indexes[idx][0];
	}
}

void MyPlot::build_plot(bool caused_by_run, Plot_Mode override_mode) {
	
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
		//if(mode == Plot_Mode::residuals || mode == Plot_Mode::residuals_histogram || mode == Plot_Mode::qq)
		//	input_gof_offset = result_gof_offset;
		//else
		input_gof_offset = steps_between(input_start, gof_start, app->time_step_size);
	}
	
	bool gof_available = false;
	bool want_gof = (bool)parent->gof_option.GetData() || mode == Plot_Mode::residuals || mode == Plot_Mode::residuals_histogram || mode == Plot_Mode::qq;
	
	s64 offset_sim;
	s64 offset_obs;
	char buf1[64];
	char buf2[64];
	gof_start.to_string(buf1);
	gof_end  .to_string(buf2);
	
	plot_info->append(Format("Showing statistics for interval %s to %s&&", buf1, buf2));
	
	try {
		Residual_Stats residual_stats;
		if(want_gof && setup.selected_results.size() == 1 && setup.selected_series.size() == 1 && !multi_index) {
			gof_available = true;
			bool compute_rcc = parent->stat_settings.display_settings.display_srcc;
			Var_Id id_sim = setup.selected_results[0];
			Var_Id id_obs = setup.selected_series[0];
			Structured_Storage<double, Var_Id> *data_sim = &app->result_data;
			Structured_Storage<double, Var_Id> *data_obs = id_obs.type == 1 ? &app->series_data : &app->additional_series_data;
			std::vector<Index_T> indexes;
			get_single_indexes(indexes, setup);
			offset_sim = data_sim->get_offset(id_sim, indexes);
			offset_obs = data_obs->get_offset(id_obs, indexes);
			compute_residual_stats(&residual_stats, data_sim, offset_sim, result_gof_offset, data_obs, offset_obs, input_gof_offset, gof_ts, compute_rcc);
		}
		
		compute_x_data(input_start, input_ts, app->time_step_size);
		
		int n_bins_histogram = 0;
		
		if(mode == Plot_Mode::regular || mode == Plot_Mode::stacked || mode == Plot_Mode::stacked_share) {
			
			if(mode != Plot_Mode::regular) {
				bool is_share = (mode == Plot_Mode::stacked_share);
				data_stacked.set_share(is_share);
			}
			
			// TODO add_plot_recursive should take the gof interval to compute stats only for
			// that interval!
			
			std::vector<Index_T> indexes(MAX_INDEX_SETS);
			for(auto var_id : setup.selected_results) {
				const std::vector<Entity_Id> &index_sets = app->result_data.get_index_sets(var_id);
				add_plot_recursive(this, app, var_id, indexes, 0, result_offset, 0, result_ts, x_data.data(), index_sets);
			}
			
			for(auto var_id : setup.selected_series) {
				const std::vector<Entity_Id> &index_sets = var_id.type == 1 ? app->series_data.get_index_sets(var_id) : app->additional_series_data.get_index_sets(var_id);
				add_plot_recursive(this, app, var_id, indexes, 0, 0, 0, input_ts, x_data.data(), index_sets);
			}
			
		} else {
			SetTitle("Plot mode not yet implemented :(");
			return;
		}

		if(gof_available) {
			String gof_title = "Goodness of fit:";
			if(mode == Plot_Mode::compare_baseline)
				gof_title = "Goodness of fit (changes rel. to baseline):";
		
			//TODO caused_by_run should instead indicate whether the data changed (either
			//re-run or different time series selection. Then always display diff if it is
			//available, but only update cache if the data changed.
			bool display_diff = caused_by_run && cached_stats.initialized;  //Whether or not to show difference between current and cached GOF stats.
			display_residual_stats(plot_info, &parent->stat_settings.display_settings, &residual_stats, &cached_stats, display_diff, gof_title);
			
			//NOTE: If in baseline mode, only cache the stats of the baseline.
			if(mode != Plot_Mode::compare_baseline || parent->baseline_was_just_saved)
				cached_stats = residual_stats;
		} else if (want_gof) {
			plot_info->append("\nTo show goodness of fit, select a single result and input series.\n");
			plot_info->ScrollEnd();
		}
		
		format_axes(this, mode, n_bins_histogram, input_start, app->time_step_size);
	
	} catch(int) {}
	parent->log_warnings_and_errors();
	
	Refresh();
}