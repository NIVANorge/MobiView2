#include "PlotCtrl.h"
#include "MobiView2.h"
#include "Statistics.h"

using namespace Upp;

std::vector<Color> Plot_Colors::colors = {{0, 130, 200}, {230, 25, 75}, {245, 130, 48}, {145, 30, 180}, {60, 180, 75},
                  {70, 240, 240}, {240, 50, 230}, {210, 245, 60}, {250, 190, 190}, {0, 128, 128}, {230, 190, 255},
                  {170, 110, 40}, {128, 0, 0}, {170, 255, 195}, {128, 128, 0}, {255, 215, 180}, {0, 0, 128}, {255, 225, 25}};


MyPlot::MyPlot() {
	//this->SetFastViewX(true); Can't be used with scatter plot data since it combines points.
	//SetSequentialXAll(true); // NOTE: with this on, lines that clip the plot area to the left
	//are culled :(   . Doesn't seem to matter that much to speed though, so just leave it off?
	
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
	RemoveSurf();
	
	colors.reset();
	
	x_data.clear();
	data_stacked.clear();
	profile.clear();
	profile2D.clear();
	histogram.Clear();
	series_data.Clear();
	
	//surf_x.Clear();
	//surf_y.Clear();
	//surf_z.Clear();
	
	SetTitle("");
	SetLabelX(" ");
	SetLabelY(" ");
	
	labels.Clear();
	
	if(full_clean)
		was_auto_resized = false;
}

void MyPlot::compute_x_data(Date_Time start, s64 steps, Time_Step_Size ts_size) {
	x_data.resize(steps);
	Expanded_Date_Time dt(start, ts_size);
	for(s64 step = 0; step < steps; ++step) {
		x_data[step] = (double)(dt.date_time.seconds_since_epoch - start.seconds_since_epoch);
		dt.advance();
	}
}

// TODO: This could maybe be a utility function in the Model_Application itself!
void get_storage_and_var(Model_Data *md, Var_Id var_id, Data_Storage<double, Var_Id> **data, State_Variable **var) {
	if(var_id.type == Var_Id::Type::state_var) {
		*data = &md->results;
		*var = md->app->state_vars[var_id];
	} else if(var_id.type == Var_Id::Type::series) {
		*data = &md->series;
		*var  = md->app->series[var_id];
	} else if(var_id.type == Var_Id::Type::additional_series) {
		*data = &md->additional_series;
		*var = md->app->additional_series[var_id];
	}
}

void format_plot(MyPlot *draw, Var_Id::Type type, Color &color, String &legend, String &unit) {
	draw->Legend(legend).Units(unit);
	if(type == Var_Id::Type::state_var || !draw->setup.scatter_inputs)
		draw->NoMark().Stroke(1.5, color).Dash("");
	else {
		draw->MarkBorderColor(color).Stroke(0.0, color).Opacity(0.5).MarkStyle<CircleMarkPlot>();
		int index = draw->GetCount()-1;
		draw->SetMarkColor(index, Null); //NOTE: Calling draw->MarkColor(Null) does not make it transparent, so we have to do it like this.
	}
}

void add_single_plot(MyPlot *draw, Model_Data *md, Model_Application *app, Var_Id var_id, std::vector<Index_T> &indexes,
	s64 ts, Date_Time ref_x_start, Date_Time start, double *x_data, s64 gof_offset, s64 gof_ts, Color &color, bool stacked,
	const String &legend_prefix, bool always_copy_y) {
		
	Data_Storage<double, Var_Id> *data;
	State_Variable *var;
	get_storage_and_var(md, var_id, &data, &var);
	s64 offset = data->structure->get_offset(var_id, indexes);
	
	String unit = var->unit.to_utf8();
	String legend = String(var->name) + " " + make_index_string(data->structure, indexes, var_id) + "[" + unit + "]";
	if(!IsNull(legend_prefix))
		legend = legend_prefix + legend;
	
	draw->series_data.Create<Agg_Data_Source>(data, offset, ts, x_data, ref_x_start, start, app->time_step_size, &draw->setup, always_copy_y);
	if(stacked) {
		draw->data_stacked.Add(draw->series_data.Top());
		draw->AddSeries(draw->data_stacked.top()).Fill(color);
	} else
		draw->AddSeries(draw->series_data.Top());
	format_plot(draw, var_id.type, color, legend, unit);
	
	if(draw->plot_info) {
		Time_Series_Stats stats;
		compute_time_series_stats(&stats, &draw->parent->stat_settings.settings, data, offset, gof_offset, gof_ts);
		display_statistics(draw->plot_info, &draw->parent->stat_settings.display_settings, &stats, color, legend);
	}
}

Date_Time
normalize_to_aggregation_period(Date_Time time, Aggregation_Period agg) {
	if(agg == Aggregation_Period::none) return time;
	Date_Time result = time;
	result.seconds_since_epoch -= result.second_of_day();
	if(agg == Aggregation_Period::weekly) {
		int dow = result.day_of_week()-1;
		result.seconds_since_epoch -= 86400*dow;
	} else {
		s32 y, m, d;
		result.year_month_day(&y, &m, &d);
		m = (agg == Aggregation_Period::yearly) ? 1 : m;
		result = Date_Time(y, m, 1);
	}
	return result;
}

void add_plot_recursive(MyPlot *draw, Model_Application *app, Var_Id var_id, std::vector<Index_T> &indexes, int level,
	Date_Time ref_x_start, Date_Time start, s64 time_steps, double *x_data, const std::vector<Entity_Id> &index_sets, s64 gof_offset, s64 gof_ts, Plot_Mode mode) {
	if(level == draw->setup.selected_indexes.size()) {

		Color &graph_color = draw->colors.next();
		bool stacked = var_id.type == Var_Id::Type::state_var && (mode == Plot_Mode::stacked || mode == Plot_Mode::stacked_share);
		add_single_plot(draw, &app->data, app, var_id, indexes, time_steps, ref_x_start, start, x_data, gof_offset, gof_ts, graph_color, stacked);

	} else {
		bool loop = false;
		if(!draw->setup.selected_indexes[level].empty()) {
			auto index_set = draw->setup.selected_indexes[level][0].index_set;
			auto find = std::find(index_sets.begin(), index_sets.end(), index_set);
			loop = find != index_sets.end();
		}
		if(!loop) {
			indexes[level] = invalid_index;
			add_plot_recursive(draw, app, var_id, indexes, level+1, ref_x_start, start, time_steps, x_data, index_sets, gof_offset, gof_ts, mode);
		} else {
			for(Index_T index : draw->setup.selected_indexes[level]) {
				indexes[level] = index;
				add_plot_recursive(draw, app, var_id, indexes, level+1, ref_x_start, start, time_steps, x_data, index_sets, gof_offset, gof_ts, mode);
			}
		}
	}
}

void set_round_grid_line_positions(ScatterDraw *plot, int axis) {
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

inline void grid_time_stamp_format(int res_type, Date_Time ref_date, double seconds_since_ref, String &str) {
	static const char *month_names[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	//NOTE: Adding 0.5 helps a bit with avoiding flickering when panning, but is not perfect (TODO)
	Date_Time d2 = ref_date;
	d2.seconds_since_epoch += (s64)(seconds_since_ref + 0.5);
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

inline void time_stamp_format(int res_type, Date_Time ref_date, double seconds_since_ref, String &str) {
	Date_Time d2 = ref_date;
	d2.seconds_since_epoch += (s64)(seconds_since_ref + 0.5);
	s32 y, mn, d, h, m, s;
	d2.year_month_day(&y, &mn, &d);
	if(res_type <= 2) {
		d2.hour_minute_second(&h, &m, &s);
		str = Format("%02d-%02d-%02d %02d:%02d:%02d", y, mn, d, h, m, s);
	} else
		str = Format("%02d-%02d-%02d", y, mn, d);
}

void format_axes(MyPlot *plot, Plot_Mode mode, int n_bins_histogram, Date_Time input_start, Time_Step_Size ts_size) {
	plot->SetGridLinesX.Clear();
	
	if(plot->GetCount() > 0 || (mode == Plot_Mode::profile2D && plot->profile2D.count() > 0)) {
		plot->cbModifFormatXGridUnits.Clear();
		plot->cbModifFormatX.Clear();
		plot->cbModifFormatY.Clear();
		plot->cbModifFormatYGridUnits.Clear();
		
		if(mode == Plot_Mode::histogram || mode == Plot_Mode::residuals_histogram) {
			//NOTE: Histograms require special zooming.
			plot->ZoomToFit(true, true);
			plot->was_auto_resized = false;
			double x_range = plot->GetXRange();
			double x_min   = plot->GetXMin();
			
			//NOTE: The auto-resize cuts out half of each outer bar, so we fix that
			double stride = x_range / (double)(n_bins_histogram-1);
			x_min   -= 0.5*stride;
			x_range += stride;
			plot->SetXYMin(x_min);
			plot->SetRange(x_range);
			
			// Position grid lines at the actual bars.
			int line_skip = n_bins_histogram / 30 + 1;
			plot->SetGridLinesX << [n_bins_histogram, stride, line_skip](Vector<double> &pos) {
				for(int idx = 0; idx < n_bins_histogram; idx+=line_skip)
					pos << stride * (double)idx;
			};
		} else if(mode == Plot_Mode::qq) {
			//NOTE: qq plots require special zooming.
			plot->ZoomToFit(true, true);
			plot->was_auto_resized = false;
			
			double y_range = plot->GetYRange();
			double y_min   = plot->GetYMin();
			double x_range = plot->GetXRange();
			double x_min   = plot->GetXMin();
			
			//NOTE: Make it so that the extremal points are not on the border
			double ext_y = y_range * 0.1;
			y_range += 2.0 * ext_y;
			y_min -= ext_y;
			double ext_x = x_range * 0.1;
			x_range += 2.0 * ext_x;
			x_min -= ext_x;
			
			plot->SetRange(x_range, y_range);
			plot->SetXYMin(x_min, y_min);
		} else if(mode == Plot_Mode::profile) {
			plot->was_auto_resized = false;
			plot->SetXYMin(0.0, 0.0);
			s64 count = plot->profile.GetCount();
			plot->SetRange((double)count, plot->profile.get_max());
			plot->SetMajorUnits(1.0);
			plot->SetMinUnits(0.5);
			plot->cbModifFormatX << [count, plot](String &s, int i, double d) {
				int idx = (int)d;
				if(d >= 0 && d < count) s = plot->labels[idx];
			};
		} else {
			plot->ZoomToFit(false, true);
			
			int res_type = compute_smallest_step_resolution(plot->setup.aggregation_period, ts_size);
			if(mode == Plot_Mode::profile2D) {
				plot->ZoomToFitZ();
				plot->SetMajorUnits(Null, 1.0);
				plot->cbModifFormatY <<
				[plot](String &s, int i, double d) {
					int idx = (int)d;
					if(d >= 0 && d < plot->labels.size()) s = plot->labels[idx];
				};
			} else {
				// Set the minimum of the y range to be 0 unless the minimum is already below 0
				double y_range = plot->GetYRange();
				double y_min   = plot->GetYMin();
				if(y_min > 0.0) {
					double new_range = y_range + y_min;
					plot->SetRange(Null, new_range);
					plot->SetXYMin(Null, 0.0);
				}
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
			plot->cbModifFormatXGridUnits << [res_type, input_start] (String &str, int i, double r) {
				grid_time_stamp_format(res_type, input_start, r, str);
			};
			
			// Format to be displayed in data table
			plot->cbModifFormatX << [res_type, input_start] (String &str, int i, double r) {
				time_stamp_format(res_type, input_start, r, str);
			};
		}
		
		// Extend the Y range a bit more to avoid the legend obscuring the plot in the most
		// common cases (and to have a nice margin).
		if(plot->GetShowLegend() && mode != Plot_Mode::profile && mode != Plot_Mode::profile2D)
			plot->SetRange(Null, plot->GetYRange() * 1.15);
		
		if(mode != Plot_Mode::profile2D) {
			if(plot->setup.y_axis_mode == Y_Axis_Mode::logarithmic) {
				plot->cbModifFormatYGridUnits << [](String &s, int i, double d) {
					s = FormatDoubleExp(pow(10., d), 2);
				};
			} else {
				plot->cbModifFormatYGridUnits << [](String &s, int i, double d) {
					s = FormatDouble(d, 4);
				};
			}
			set_round_grid_line_positions(plot, 1);
		}

		if(mode == Plot_Mode::qq)
			set_round_grid_line_positions(plot, 0);
	}
	
	bool allow_scroll_x = !(mode == Plot_Mode::profile || mode == Plot_Mode::histogram || mode == Plot_Mode::residuals_histogram);
	bool allow_scroll_y = (mode == Plot_Mode::qq);
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

void add_line(MyPlot *plot, double x0, double y0, double x1, double y1, Color color, const String &legend) {
	plot->series_data.Create<Data_Source_Line>(x0, y0, x1, y1);
	if(IsNull(color)) color = plot->colors.next();
	plot->AddSeries(plot->series_data.Top()).NoMark().Stroke(1.5, color).Dash("6 3");
	if(!IsNull(legend)) plot->Legend(legend);
	else plot->ShowSeriesLegend(false);
}

void add_trend_line(MyPlot *plot, double xy_covar, double x_var, double y_mean, double x_mean, double start_x, double end_x, String &legend) {
	double beta = xy_covar / x_var;
	double alpha = y_mean - beta*x_mean;

	add_line(plot, start_x, alpha + start_x*beta, end_x, alpha + end_x*beta, Null, legend);
}

int add_histogram(MyPlot *plot, DataSource *data, double min, double max, s64 count, String &legend, String &unit, Color &color) {
	//n_bins_histogram = 1 + (int)std::ceil(std::log2((double)count));      //NOTE: Sturges' rule.
	int n_bins_histogram = 2*(int)(std::ceil(std::cbrt((double)count)));    //NOTE: Rice's rule.
	
	plot->histogram.Create(*data, min, max, n_bins_histogram);
	plot->histogram.Normalize();
	
	double stride = (max - min)/(double)n_bins_histogram;
	double darken = 0.4;
	Color border((int)(((double)color.GetR())*darken), (int)(((double)color.GetG())*darken), (int)(((double)color.GetB())*darken));
	plot->AddSeries(plot->histogram).Legend(legend).PlotStyle<BarSeriesPlot>().BarWidth(0.5*stride).NoMark().Fill(color).Stroke(1.0, border).Units("", unit);
	
	return n_bins_histogram;
}

void get_gof_offsets(Time &start_setting, Time &end_setting, Date_Time input_start, s64 input_ts, Date_Time result_start, s64 result_ts, Date_Time &gof_start, Date_Time &gof_end,
	s64 &input_gof_offset, s64 &result_gof_offset, s64 &gof_ts, Time_Step_Size ts_size, bool has_results) {

	Date_Time gof_min = result_start;
	s64 max_ts = result_ts;
	if(!has_results) {
		gof_min = input_start;
		max_ts  = input_ts;
	}
	Date_Time gof_max = advance(gof_min, ts_size, max_ts-1);
	
	gof_start = IsNull(start_setting) ? gof_min : convert_time(start_setting);
	gof_end   = IsNull(end_setting)   ? gof_max : convert_time(end_setting);
	
	if(gof_start < gof_min || gof_start > gof_max)
		gof_start = gof_min;
	if(gof_end   < gof_min || gof_end   > gof_max || gof_end < gof_start)
		gof_end   = gof_max;
	
	gof_ts = steps_between(gof_start, gof_end, ts_size) + 1; //NOTE: if start time = end time, there is still one timestep.
	result_gof_offset = steps_between(result_start, gof_start, ts_size); //NOTE: this could be negative, but only in the case when has_results=false
	input_gof_offset = steps_between(input_start, gof_start, ts_size);
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
	
	Date_Time result_start = app->data.results.start_date;
	Date_Time input_start  = app->data.series.start_date;  // NOTE: Should be same for both type of series (model inputs & additional series)
	
	s64 input_ts = app->data.series.time_steps;
	s64 result_ts = app->data.results.time_steps;
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
	
	Time gof_start_setting  = parent->calib_start.GetData();
	Time gof_end_setting    = parent->calib_end.GetData();
	get_gof_offsets(gof_start_setting, gof_end_setting, input_start, input_ts, result_start, result_ts, gof_start, gof_end,
		input_gof_offset, result_gof_offset, gof_ts, app->time_step_size, !setup.selected_results.empty());
	
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
			Data_Storage<double, Var_Id> *data_sim = &app->data.results;
			Data_Storage<double, Var_Id> *data_obs = id_obs.type == Var_Id::Type::series ? &app->data.series : &app->data.additional_series;
			std::vector<Index_T> indexes;
			get_single_indexes(indexes, setup);
			offset_sim = data_sim->structure->get_offset(id_sim, indexes);
			offset_obs = data_obs->structure->get_offset(id_obs, indexes);
			compute_residual_stats(&residual_stats, data_sim, offset_sim, result_gof_offset, data_obs, offset_obs, input_gof_offset, gof_ts, compute_rcc);
		}
		
		compute_x_data(input_start, input_ts+1, app->time_step_size);
		
		int n_bins_histogram = 0;
		
		if(mode == Plot_Mode::regular || mode == Plot_Mode::stacked || mode == Plot_Mode::stacked_share) {
			
			if(mode != Plot_Mode::regular) {
				bool is_share = (mode == Plot_Mode::stacked_share);
				data_stacked.set_share(is_share);
			}
			
			std::vector<Index_T> indexes(MAX_INDEX_SETS);
			for(auto var_id : setup.selected_results) {
				const std::vector<Entity_Id> &index_sets = app->result_structure.get_index_sets(var_id);
				add_plot_recursive(this, app, var_id, indexes, 0, input_start, result_start, result_ts, x_data.data(), index_sets, result_gof_offset, gof_ts, mode);
			}
			
			for(auto var_id : setup.selected_series) {
				const std::vector<Entity_Id> &index_sets = var_id.type == Var_Id::Type::series ? app->series_structure.get_index_sets(var_id) : app->additional_series_structure.get_index_sets(var_id);
				add_plot_recursive(this, app, var_id, indexes, 0, input_start, input_start, input_ts, x_data.data(), index_sets, input_gof_offset, gof_ts, mode);
			}
		} else if (mode == Plot_Mode::histogram) {
			
			if(series_count > 1 || multi_index) {
				SetTitle("In histogram mode you can only have one timeseries selected, for one index combination");
				return;
			}
			
			std::vector<Index_T> indexes;
			get_single_indexes(indexes, setup);
			
			s64 gof_offset;
			Var_Id var_id;
			
			// TODO: We should make it very clear that the histogram uses the GOF interval only
			// for its data!
			
			if(!setup.selected_results.empty()) {
				var_id = setup.selected_results[0];
				gof_offset  = result_gof_offset;
			} else {
				var_id = setup.selected_series[0];
				gof_offset  = input_gof_offset;
			}
			
			Data_Storage<double, Var_Id> *data;
			State_Variable *var;
			get_storage_and_var(&app->data, var_id, &data, &var);
			
			//TODO: with the new data system, it would be easy to allow aggregation also.
			s64 offset = data->structure->get_offset(var_id, indexes);
			Time_Series_Stats stats;
			compute_time_series_stats(&stats, &parent->stat_settings.settings, data, offset, gof_offset, gof_ts);
			
			series_data.Create<Mobius_Data_Source>(data, offset, gof_ts, x_data.data(), input_start, gof_start, app->time_step_size);
			
			String unit = var->unit.to_utf8();
			String legend = String(var->name) + " " + make_index_string(data->structure, indexes, var_id) + "[" + unit + "]";
			
			Color &color = colors.next();
			n_bins_histogram = add_histogram(this, &series_data.Top(), stats.min, stats.max, stats.data_points, legend, unit, color);
			display_statistics(plot_info, &parent->stat_settings.display_settings, &stats, color, legend);
			
		} else if(mode == Plot_Mode::residuals || mode == Plot_Mode::residuals_histogram || mode == Plot_Mode::qq) {
			if(!gof_available) {
				this->SetTitle("In residual mode you must select exactly 1 result series and 1 input series, for one index combination only");
				return;
			}
			
			std::vector<Index_T> indexes;
			get_single_indexes(indexes, setup);
			
			Var_Id var_id_sim = setup.selected_results[0];
			Var_Id var_id_obs = setup.selected_series[0];
			Data_Storage<double, Var_Id> *data_sim, *data_obs;
			State_Variable *var_sim, *var_obs;
			get_storage_and_var(&app->data, var_id_sim, &data_sim, &var_sim);
			get_storage_and_var(&app->data, var_id_obs, &data_obs, &var_obs);
			
			s64 offset_sim = data_sim->structure->get_offset(var_id_sim, indexes);
			s64 offset_obs = data_obs->structure->get_offset(var_id_obs, indexes);
			
			// TODO: make some kind of check that the units match?
			String unit = var_sim->unit.to_utf8();
			String obs_unit = var_obs->unit.to_utf8();
			String legend_obs = String(var_obs->name) + " " + make_index_string(data_obs->structure, indexes, var_id_obs) + "[" + obs_unit + "]";
			String legend_sim = String(var_sim->name) + " " + make_index_string(data_sim->structure, indexes, var_id_sim) + "[" + unit + "]";
			String legend = String("Residuals of ") + legend_obs + " vs. " + legend_sim;
			
			Time_Series_Stats obs_stats;
			compute_time_series_stats(&obs_stats, &parent->stat_settings.settings, data_obs, offset_obs, input_gof_offset, gof_ts);
			display_statistics(plot_info, &parent->stat_settings.display_settings, &obs_stats, Black(), legend_obs);
			
			Time_Series_Stats sim_stats;
			compute_time_series_stats(&sim_stats, &parent->stat_settings.settings, data_sim, offset_sim, result_gof_offset, gof_ts);
			display_statistics(plot_info, &parent->stat_settings.display_settings, &sim_stats, Black(), legend_sim);
			
			if(mode == Plot_Mode::residuals) {
				series_data.Create<Agg_Data_Source>(data_sim, data_obs, offset_sim, offset_obs, result_ts, x_data.data(),
					input_start, result_start, app->time_step_size, &setup);
					
				AddSeries(series_data.Top());
				format_plot(this, Var_Id::Type::series, colors.next(), legend, unit);
				
				double first_x = (double)(gof_start.seconds_since_epoch - input_start.seconds_since_epoch);
				double last_x  = (double)(gof_end  .seconds_since_epoch - input_start.seconds_since_epoch);
				
				double xy_covar, x_var, y_mean, x_mean;
				// Hmm, this will make a trend of the aggregated data. Should we do it that way?
				compute_trend_stats(&series_data.Top(), x_mean, y_mean, x_var, xy_covar);
				add_trend_line(this, xy_covar, x_var, y_mean, x_mean, first_x, last_x, String("Trend line"));
				
				add_line(this, first_x, residual_stats.min_error, first_x, residual_stats.max_error, Red(), Null);
				add_line(this, last_x,  residual_stats.min_error, last_x,  residual_stats.max_error, Red(), Null);
			} else if(mode == Plot_Mode::residuals_histogram) {
				//TODO : Could actually allow aggrecation for this now
				series_data.Create<Residual_Data_Source>(data_sim, data_obs, offset_sim, offset_obs, gof_ts, x_data.data(),
					input_start, gof_start, app->time_step_size);

				n_bins_histogram = add_histogram(this, &series_data.Top(), residual_stats.min_error, residual_stats.max_error, residual_stats.data_points, legend, unit, colors.next());
				
				// Create a normal distribution with the same mean and std_dev
				double mean    = series_data.Top().AvgY();
				double std_dev = series_data.Top().StdDevY(mean);
				std::vector<double> xx(n_bins_histogram), yy(n_bins_histogram);
				double stride = (residual_stats.max_error - residual_stats.min_error) / (double)n_bins_histogram;
				for(int pt = 0; pt < n_bins_histogram; ++pt) {
					double low = residual_stats.min_error + (double)pt * stride;
					double high = low + stride;
					xx[pt] = 0.5*(low + high);
					yy[pt] = 0.5 * (std::erf((high-mean) / (std::sqrt(2.0)*std_dev)) - std::erf((low-mean) / (std::sqrt(2.0)*std_dev)));
				}
				series_data.Create<Data_Source_Owns_XY>(&xx, &yy);
				Color &color = colors.next();
				AddSeries(series_data.Top()).Legend("Normal distr.").MarkColor(color).Stroke(0.0, color).Dash("");

			} else if(mode == Plot_Mode::qq) {
				auto &perc = parent->stat_settings.settings.percentiles;
				int num_perc = perc.size();
				std::vector<double> xx(num_perc), yy(num_perc);
				for(int idx = 0; idx < num_perc; ++idx) {
					xx[idx] = sim_stats.percentiles[idx];
					yy[idx] = obs_stats.percentiles[idx];
					labels << Format(" %g", perc[idx]);
				}
				Color color = colors.next();
				series_data.Create<Data_Source_Owns_XY>(&xx, &yy);
				AddSeries(series_data.Top()).MarkColor(color).Stroke(0.0, color).Dash("").Units(unit, unit)
					.AddLabelSeries(labels, 10, 0, StdFont().Height(15), ALIGN_LEFT);
				ShowLegend(false);
				
				SetLabels(legend_sim, legend_obs); //TODO: This does not work!
				double min = std::min(sim_stats.percentiles[0], obs_stats.percentiles[0]);
				double max = std::max(sim_stats.percentiles[num_perc-1], obs_stats.percentiles[num_perc-1]);

				add_line(this, min, min, max, max, Red(), Null);
			}
		} else if (mode == Plot_Mode::profile || mode == Plot_Mode::profile2D) {
			
			int profile_set_idx;
			int index_count;
			int n_multi_index = 0;
			for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
				int count = setup.selected_indexes[idx].size();
				if(count > 1 && setup.index_set_is_active[idx]) {
					n_multi_index++;
					index_count = count;
					profile_set_idx = idx;
				}
			}
			if(series_count != 1 || n_multi_index != 1) {
				SetTitle("In profile mode you can only have one timeseries selected, and exactly one index set must have multiple indexes selected");
				return;
			}
			Var_Id var_id;
			s64 steps;
			Date_Time start;
			if(!setup.selected_results.empty()) {
				var_id = setup.selected_results[0];
				steps = result_ts;
				start = result_start;
			} else {
				var_id = setup.selected_series[0];
				steps = input_ts;
				start = input_start;
			}
			Data_Storage<double, Var_Id> *data;
			State_Variable *var;
			get_storage_and_var(&app->data, var_id, &data, &var);
			profile_unit   = var->unit.to_utf8();
			profile_legend = String(var->name) + "[" + profile_unit + "]";
			for(Index_T &index : setup.selected_indexes[profile_set_idx])
				labels << String(app->index_names[profile_set_idx][index.index]);
			
			std::vector<Index_T> indexes;
			get_single_indexes(indexes, setup);
			
			for(Index_T &index : setup.selected_indexes[profile_set_idx]) {
				indexes[profile_set_idx] = index;
				s64 offset = data->structure->get_offset(var_id, indexes);
				series_data.Create<Agg_Data_Source>(data, offset, steps, x_data.data(), input_start, start, app->time_step_size, &setup);
			}
			
			if(mode == Plot_Mode::profile) {
				
				int idx = 0;
				for(Index_T &index : setup.selected_indexes[profile_set_idx])
					profile.add(&series_data[idx++]);

				s64 agg_steps = series_data[0].GetCount();
				
				if(plot_ctrl) {
					Date_Time prof_start = normalize_to_aggregation_period(start, setup.aggregation_period);
					plot_ctrl->profile_base_time = prof_start;
					plot_ctrl->time_step_edit.SetData(convert_time(prof_start));
					plot_ctrl->time_step_slider.Range(agg_steps-1);
					setup.profile_time_step = plot_ctrl->time_step_slider.GetData();
					plot_ctrl->time_step_slider.Enable();
					// NOTE: this condition is only because we didn't implement functionality for it when it is aggregated
					if(setup.aggregation_period == Aggregation_Period::none)
						plot_ctrl->time_step_edit.Enable();
				}
				
				Color &color = colors.next();
				double darken = 0.4;
				Color border_color((int)(((double)color.GetR())*darken), (int)(((double)color.GetG())*darken), (int)(((double)color.GetB())*darken));
				AddSeries(profile).Legend(profile_legend).PlotStyle<BarSeriesPlot>().BarWidth(0.5).NoMark().Fill(color).Stroke(1.0, border_color).Units(profile_unit);
			} else if (mode == Plot_Mode::profile2D) {
				int idx = 0;
				for(Index_T &index : setup.selected_indexes[profile_set_idx])
					profile2D.add(&series_data[idx++]);
				AddSurf(profile2D);
			}
		} else if (mode == Plot_Mode::compare_baseline) {
			if(setup.selected_results.size() > 1 || multi_index) {
				SetTitle("In baseline comparison mode you can only have one result series selected, for one index combination");
				return;
			} else if(!parent->baseline) {
				SetTitle("The baseline comparison can only be displayed if the baseline has been saved (using a button in the toolbar)");
				return;
			}
			
			auto baseline = parent->baseline;
			s64 baseline_ts = baseline->results.time_steps;
			Date_Time baseline_start = baseline->results.start_date;
			
			std::vector<Index_T> indexes;
			get_single_indexes(indexes, setup);
			
			//NOTE: to get same color for main series and comparison series as when in regular
			//plot, we extract colors in this order.
			Color &main_col = colors.next();
			Color &ser_col  = colors.next();
			Color &bl_col   = colors.next();
			//TODO: factor out some stuff here:
			auto var_id = setup.selected_results[0];
			
			add_single_plot(this, &app->data, app, var_id, indexes, result_ts, input_start, result_start, x_data.data(),
				result_gof_offset, gof_ts, main_col);
			
			//TODO: result_gof_offset may not be correct for the baseline!
			add_single_plot(this, baseline, app, var_id, indexes, baseline_ts, input_start, baseline_start, x_data.data(),
				result_gof_offset, gof_ts, bl_col, false, "baseline of ");
		
			if(setup.selected_series.size() == 1) {
				auto var_id = setup.selected_series[0];
				add_single_plot(this, &app->data, app, var_id, indexes, input_ts, input_start, input_start, x_data.data(),
					input_gof_offset, gof_ts, ser_col);
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
	
	if(mode == Plot_Mode::profile) {
		replot_profile();
	} else
		Refresh();
}

void MyPlot::replot_profile() {
	profile.set_ts(setup.profile_time_step);
	Refresh();
}

void
aggregate_data(Date_Time &ref_time, Date_Time &start_time, DataSource *data,
	Aggregation_Period agg_period, Aggregation_Type agg_type, Time_Step_Size ts_size, std::vector<double> &x_vals, std::vector<double> &y_vals)
{
	s64 steps = data->GetCount();
	
	double curr_agg;
	if(agg_type == Aggregation_Type::mean || agg_type == Aggregation_Type::sum)
		curr_agg = 0.0;
	else if(agg_type == Aggregation_Type::min)
		curr_agg = std::numeric_limits<double>::infinity();
	else if(agg_type == Aggregation_Type::max)
		curr_agg = -std::numeric_limits<double>::infinity();
		
	int count = 0;
	Expanded_Date_Time time(start_time, ts_size);
	
	for(s64 step = 0; step < steps; ++step) {
		double val = data->y(step);
		
		//TODO: more numerically stable summation method.
		if(std::isfinite(val)) {
			if(agg_type == Aggregation_Type::mean || agg_type == Aggregation_Type::sum)
				curr_agg += val;
			else if(agg_type == Aggregation_Type::min)
				curr_agg = std::min(val, curr_agg);
			else if(agg_type == Aggregation_Type::max)
				curr_agg = std::max(val, curr_agg);
			++count;
		}
		
		Date_Time prev = time.date_time;
		s32 y = time.year;
		s32 m = time.month;
		s32 d = time.day_of_month;
		s32 w = week_of_year(time.day_of_year);
		time.advance();
		
		//TODO: Want more aggregation interval types than year or month for models with
		//non-daily resolutions
		bool push = (step == steps-1);
		if(agg_period == Aggregation_Period::yearly || agg_period == Aggregation_Period::monthly)
			push = push || (time.year != y);
		if(agg_period == Aggregation_Period::monthly)
			push = push || (time.month != m);
		if(agg_period == Aggregation_Period::weekly)
			push = push || (week_of_year(time.day_of_year) != w);
		
		if(push) {
			double yval = curr_agg;
			if(agg_type == Aggregation_Type::mean) yval /= (double)count;
			if(!std::isfinite(yval) || count == 0) yval = std::numeric_limits<double>::quiet_NaN();
			y_vals.push_back(yval);
			
			// Put the mark down at a round location.
			Date_Time mark = normalize_to_aggregation_period(prev, agg_period);
			double xval = (double)(mark.seconds_since_epoch - ref_time.seconds_since_epoch);
			x_vals.push_back(xval);
			
			if(agg_type == Aggregation_Type::mean || agg_type == Aggregation_Type::sum)
				curr_agg = 0.0;
			else if(agg_type == Aggregation_Type::min)
				curr_agg = std::numeric_limits<double>::infinity();
			else if(agg_type == Aggregation_Type::max)
				curr_agg = -std::numeric_limits<double>::infinity();
			
			count = 0;
		}
	}
}