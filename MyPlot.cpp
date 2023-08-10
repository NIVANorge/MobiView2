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

	SetRainbowPalettePos({50, 10});
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
	profile2Dt.clear();
	profile2D_is_timed = false;
	histogram.Clear();
	series_data.Clear();
	
	SetTitle("");
	SetLabelX(" ");
	SetLabelY(" ");
	
	labels.Clear();
	labels2.Clear();
	
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

void format_plot(MyPlot *draw, Var_Id::Type type, DataSource *data, Color &color, String &legend, String &unit) {
	draw->Legend(legend).Units(unit);
	
	bool scatter = false;
	if(type != Var_Id::Type::state_var && draw->setup.scatter_inputs) {
		
		bool prev_prev_valid = false;
		bool prev_valid = false;
		// See if there are any isolated points.
		for(int64 id = 0; id < data->GetCount(); ++id) {
			double y = data->y(id);
			bool valid = std::isfinite(y);
			bool edge = id==0 || id==data->GetCount()-1;
			if(!valid && prev_valid && (edge || !prev_prev_valid)) {
				scatter = true;
				break;
			}
			prev_prev_valid = prev_valid;
			prev_valid = valid;
		}
	}
	
	if(!scatter)
		draw->NoMark().Stroke(1.5, color).Dash("");
	else {
		draw->MarkBorderColor(color).Stroke(0.0, color).Opacity(0.5).MarkStyle<CircleMarkPlot>();
		int index = draw->GetCount()-1;
		draw->SetMarkColor(index, Null); //NOTE: Calling draw->MarkColor(Null) does not make it transparent, so we have to do it like this.
	}
}

bool add_single_plot(MyPlot *draw, Model_Data *md, Model_Application *app, Var_Id var_id, Indexes &indexes,
	s64 ts, Date_Time ref_x_start, Date_Time start, double *x_data, s64 gof_offset, s64 gof_ts, Color &color, bool stacked,
	const String &legend_prefix, bool always_copy_y) {
	
	if(!app->is_in_bounds(indexes)) return true;
	
	if(draw->GetCount() == 100) {
		draw->SetTitle("Warning: only displaying the 100 first selected series");
		return false;
	}
	
	auto *data = &md->get_storage(var_id.type);
	auto var = app->vars[var_id];
	s64 offset = data->structure->get_offset(var_id, indexes);
	
	auto unit = var->unit;
	if(draw->setup.aggregation_type == Aggregation_Type::sum)
		unit = unit_of_sum(var->unit, app->time_step_unit, draw->setup.aggregation_period);
	
	String unit_str = unit.to_utf8();
	String legend = String(var->name) + " " + make_index_string(data->structure, indexes, var_id) + "[" + unit_str + "]";
	if(!IsNull(legend_prefix))
		legend = legend_prefix + legend;
	
	draw->series_data.Create<Agg_Data_Source>(data, offset, ts, x_data, ref_x_start, start, app->time_step_size, &draw->setup, always_copy_y);
	if(stacked) {
		draw->data_stacked.Add(draw->series_data.Top());
		draw->AddSeries(draw->data_stacked.top()).Fill(color);
	} else
		draw->AddSeries(draw->series_data.Top());
	
	format_plot(draw, var_id.type, &draw->series_data.Top(), color, legend, unit_str);
	
	if(draw->plot_info) {
		Time_Series_Stats stats;
		compute_time_series_stats(&stats, &draw->parent->stat_settings.settings, data, offset, gof_offset, gof_ts);
		display_statistics(draw->plot_info, &draw->parent->stat_settings.display_settings, &stats, color, legend);
	}
	
	return true;
}

bool add_plot_recursive(MyPlot *draw, Model_Application *app, Var_Id var_id, Indexes &indexes, int level,
	Date_Time ref_x_start, Date_Time start, s64 time_steps, double *x_data, const std::vector<Entity_Id> &index_sets, s64 gof_offset, s64 gof_ts, Plot_Mode mode) {
	if(level == indexes.indexes.size()) {

		bool stacked = var_id.type == Var_Id::Type::state_var && (mode == Plot_Mode::stacked || mode == Plot_Mode::stacked_share);
		
		int sz = index_sets.size();
		if(sz >= 2 && index_sets[sz-1] == index_sets[sz-2]) {
			// If it is matrix indexed, we also loop over the final index set one more time.
			// TODO: Could this code be made a bit cleaner?
			auto set = index_sets[sz-1];
			
			indexes.mat_col.index_set = set;
			int count = app->get_index_count(set, indexes).index;
			for(int idx = 0; idx < count; ++idx) {
				if(indexes.indexes[sz-2].index == idx) continue; // Skip for when the two last indexes are the same (the corresponding series is not computed).
				indexes.mat_col.index = idx;
				
				Color &graph_color = draw->colors.next();
				bool success = add_single_plot(draw, &app->data, app, var_id, indexes, time_steps,
					ref_x_start, start, x_data, gof_offset, gof_ts, graph_color, stacked, "", false);
				if(!success) return false;		
			}
			
		} else {
			
			// This is the vast majority of cases.
			Color &graph_color = draw->colors.next();
			bool success = add_single_plot(draw, &app->data, app, var_id, indexes, time_steps,
				ref_x_start, start, x_data, gof_offset, gof_ts, graph_color, stacked);
			return success;
		}
	} else {
		bool loop = false;
		if(!draw->setup.selected_indexes[level].empty()) {
			auto index_set = draw->setup.selected_indexes[level][0].index_set;
			auto find = std::find(index_sets.begin(), index_sets.end(), index_set);
			loop = find != index_sets.end();
		}
		if(!loop) {
			indexes.indexes[level] = invalid_index;
			return add_plot_recursive(draw, app, var_id, indexes, level+1, ref_x_start, start, time_steps, x_data, index_sets, gof_offset, gof_ts, mode);
		} else {
			for(Index_T index : draw->setup.selected_indexes[level]) {
				indexes.indexes[level] = index;
				bool success = add_plot_recursive(draw, app, var_id, indexes, level+1, ref_x_start, start, time_steps, x_data, index_sets, gof_offset, gof_ts, mode);
				if(!success) return false;
			}
		}
	}
	return true;
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
			if(ts_size.multiplier < 60)         return 0;
			else if(ts_size.multiplier < 3600)  return 1;
			else if(ts_size.multiplier < 86400) return 2;
			else                               return 3;
		} else {
			if(ts_size.multiplier < 12)         return 4;
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
	
	plot->cbModifFormatXGridUnits.Clear();
	plot->cbModifFormatX.Clear();
	plot->cbModifFormatY.Clear();
	plot->cbModifFormatYGridUnits.Clear();
	
	if(plot->GetCount() > 0
		|| (mode == Plot_Mode::profile2D &&
			((!plot->profile2D_is_timed && plot->profile2D.count() > 0) ||
			(plot->profile2D_is_timed && plot->profile2Dt.count() > 0))
			)) {
		
		if(mode == Plot_Mode::histogram || mode == Plot_Mode::residuals_histogram) {
			//NOTE: Histograms require special zooming.
			plot->ZoomToFitNonLinked(true, true, 0, 0);
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
			
			// Skip some of the numeric labels since they tend to overlap otherwise.
			plot->cbModifFormatXGridUnits << [](String &s, int i, double d) {
				if(i % 2 == 1)
					s = FormatDouble(d, 3);
				else
					s = "";
			};
		} else if(mode == Plot_Mode::qq) {
			//NOTE: qq plots require special zooming.
			plot->ZoomToFitNonLinked(true, true, 0, 0);
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
			double ymin = std::min(0.0, plot->profile.get_min());
			double ymax = plot->profile.get_max();
			plot->SetXYMin(0.0, ymin);
			int count = plot->profile.GetCount();
			plot->SetRange((double)count, ymax - ymin);
			int preferred_max_grid = 15;  //TODO: Dynamic size-based ?
			int units = std::max(1, count / preferred_max_grid);
			plot->SetMajorUnits((double)units);
			plot->SetMinUnits(0.5);
			plot->cbModifFormatX << [count, plot](String &s, int i, double d) {
				int idx = (int)d;
				if(d >= 0 && d < count && (idx < plot->labels.size())) s = plot->labels[idx];
			};
		} else if (mode == Plot_Mode::profile2D && plot->profile2D_is_timed) {
			plot->ZoomToFitNonLinked(true, true, 0, 0);
			plot->was_auto_resized = false;
			plot->SetSurfMinZ(plot->profile2Dt.get_min());
			plot->SetSurfMaxZ(plot->profile2Dt.get_max());
			
			plot->SetXYMin(0.0, 0.0);
			int count_x = plot->profile2Dt.get_dim_x();
			int count_y = plot->profile2Dt.get_dim_y();
			plot->SetRange((double)count_x, (double)count_y);
			int preferred_max_grid = 15;  //TODO: Dynamic size-based ?
			int units_x = std::max(1, count_x / preferred_max_grid);
			int units_y = std::max(1, count_y / preferred_max_grid);
			plot->SetMinUnits(0.5, 0.5);
			plot->SetMajorUnits((double)units_x, (double)units_y);

			plot->cbModifFormatX << [count_x, plot](String &s, int i, double d) {
				int idx = (int)d;
				if(d >= 0 && d < count_x && (idx < plot->labels2.size())) s = plot->labels2[idx];
			};
			plot->cbModifFormatY << [count_y, plot](String &s, int i, double d) {
				int idx = (int)d;
				if(d >= 0 && d < count_y && (idx < plot->labels.size())) s = plot->labels[idx];
			};
			
		} else if (!plot->profile2D_is_timed) {
			plot->ZoomToFitNonLinked(false, true, 0, 0);
			
			int res_type = compute_smallest_step_resolution(plot->setup.aggregation_period, ts_size);
			if(mode == Plot_Mode::profile2D) {
				plot->ZoomToFitZ();
				int count = plot->profile2D.count();
				int preferred_max_grid = 15;  //TODO: Dynamic size-based ?
				int units = std::max(1, count / preferred_max_grid);
				plot->SetMinUnits(Null, 0.5);
				plot->SetMajorUnits(Null, units);
				plot->cbModifFormatY <<
				[plot](String &s, int i, double d) {
					int idx = (int)d;
					if(d >= 0 && d < plot->labels.size()) s = plot->labels[idx]; // NOTE: We display them from top to bottom
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
				plot->ZoomToFitNonLinked(true, false, 0, 0);
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
	
	bool allow_scroll_x = !(mode == Plot_Mode::profile || mode == Plot_Mode::histogram
		|| mode == Plot_Mode::residuals_histogram);
	bool allow_scroll_y = (mode == Plot_Mode::qq) || (mode == Plot_Mode::profile2D && plot->profile2D_is_timed);
	plot->SetMouseHandling(allow_scroll_x, allow_scroll_y);
}

void get_single_indexes(Indexes &indexes, Plot_Setup &setup) {
	
	if(indexes.lookup_ordered)
		fatal_error(Mobius_Error::internal, "Can't use get_single_indexes with lookup ordered indexes");
	
	//indexes.clear(); // Probably unnecessary since we only use it for newly constructed ones
	
	for(int idx = 0; idx < setup.selected_indexes.size(); ++idx) {
		if(!setup.selected_indexes[idx].empty())
			indexes.set_index(setup.selected_indexes[idx][0]);
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

int add_histogram(MyPlot *plot, DataSource *data, double min, double max, s64 count, const String &legend, const String &unit, const Color &color) {
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
			Indexes indexes(parent->model);
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
			
			Indexes indexes(parent->model);
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
			
			Indexes indexes(parent->model);
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
			
			auto *data = &app->data.get_storage(var_id.type);
			auto var = app->vars[var_id];
			
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
			
			Indexes indexes(parent->model);
			get_single_indexes(indexes, setup);
			
			Var_Id var_id_sim = setup.selected_results[0];
			Var_Id var_id_obs = setup.selected_series[0];
			auto *data_sim = &app->data.get_storage(var_id_sim.type);
			auto *data_obs = &app->data.get_storage(var_id_obs.type);
			auto var_sim = app->vars[var_id_sim];
			auto var_obs = app->vars[var_id_obs];
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
				format_plot(this, Var_Id::Type::series, &series_data.Top(), colors.next(), legend, unit);
				
				double first_x = (double)(gof_start.seconds_since_epoch - input_start.seconds_since_epoch);
				double last_x  = (double)(gof_end  .seconds_since_epoch - input_start.seconds_since_epoch);
				
				double xy_covar, x_var, y_mean, x_mean;
				// Hmm, this will make a trend of the aggregated data. Should we do it that way?
				compute_trend_stats(&series_data.Top(), x_mean, y_mean, x_var, xy_covar);
				
				double slope_days = (xy_covar / x_var)*86400;  // TODO: Ideally scale it to the time step unit, but it is a bit tricky.
				auto trend_legend = Format("Trend line. Slope: %g (%s day⁻¹)", slope_days, unit);
				add_trend_line(this, xy_covar, x_var, y_mean, x_mean, first_x, last_x, trend_legend);
				
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
			int profile_set_idx2;
			int n_multi_index = 0;
			for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
				int count = setup.selected_indexes[idx].size();
				if(count > 1 && setup.index_set_is_active[idx]) {
					if(n_multi_index == 0)
						profile_set_idx = idx;
					else
						profile_set_idx2 = idx;
					++n_multi_index;
				}
			}
			if(n_multi_index == 2) {
				this->profile2D_is_timed = true;
				std::swap(profile_set_idx, profile_set_idx2);
			}
			
			if(mode == Plot_Mode::profile && (series_count != 1 || n_multi_index != 1)) {
				SetTitle("In profile mode you can only have one timeseries selected, and exactly one index set must have multiple indexes selected");
				return;
			}
			if(mode == Plot_Mode::profile2D && (series_count != 1 || (n_multi_index != 1 && n_multi_index != 2))) {
				SetTitle("In profile2D mode you can only have one timeseries selected, and one or two index sets must have multiple indexes selected");
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

			auto *data = &app->data.get_storage(var_id.type);
			auto var = app->vars[var_id];
			
			profile_unit   = var->unit.to_utf8();
			profile_legend = String(var->name) + "[" + profile_unit + "]";
			
			Indexes indexes(parent->model);
			get_single_indexes(indexes, setup);
			
			for(Index_T &index : setup.selected_indexes[profile_set_idx]) {
				indexes.set_index(index, true);
				labels << String(app->get_index_name(index));
				
				if(!profile2D_is_timed) {
					if(!app->is_in_bounds(indexes)) continue; //TODO: should maybe only trim off the ones on the beginning or end, not the ones inside..
					s64 offset = data->structure->get_offset(var_id, indexes);
					series_data.Create<Agg_Data_Source>(data, offset, steps, x_data.data(), input_start, start, app->time_step_size, &setup);
				} else {

					for(Index_T &index2 : setup.selected_indexes[profile_set_idx2]) {
						indexes.set_index(index2, true);
						s64 offset = data->structure->get_offset(var_id, indexes);
						series_data.Create<Agg_Data_Source>(data, offset, steps, x_data.data(), input_start, start, app->time_step_size, &setup);
						labels2 << String(app->get_index_name(index2));
					}
				}
			}
			if(mode == Plot_Mode::profile2D && !profile2D_is_timed)
				std::reverse(labels.begin(), labels.end()); // We display the data from top to bottom
			
			s64 agg_steps = series_data[0].GetCount();
			
			if(plot_ctrl && (mode == Plot_Mode::profile || profile2D_is_timed)) {
				Date_Time prof_start = normalize_to_aggregation_period(start, setup.aggregation_period, setup.pivot_month);
				plot_ctrl->profile_base_time = prof_start;
				plot_ctrl->time_step_edit.SetData(convert_time(prof_start));
				plot_ctrl->time_step_slider.Range(agg_steps-1);
				setup.profile_time_step = plot_ctrl->time_step_slider.GetData();
				plot_ctrl->time_step_slider.Enable();
				// NOTE: this condition is only because we didn't implement functionality for it when it is aggregated
				if(setup.aggregation_period == Aggregation_Period::none)
					plot_ctrl->time_step_edit.Enable();
				
				if(mode == Plot_Mode::profile2D) {
					//NOTE: need to do this since the plot_ctrl couldn't detect on its own if
					//it should be in the timed mode.
					plot_ctrl->time_step_slider.Show();
					plot_ctrl->time_step_edit.Show();
					plot_ctrl->push_play.Show();
					plot_ctrl->push_rewind.Show();
				}
			} else if (!profile2D_is_timed) {
				plot_ctrl->time_step_slider.Hide();
				plot_ctrl->time_step_edit.Hide();
				plot_ctrl->push_play.Hide();
				plot_ctrl->push_rewind.Hide();
			}
			
			if(mode == Plot_Mode::profile) {
				
				int idx = 0;
				for(auto &series : series_data)
					profile.add(&series);

				Color &color = colors.next();
				double darken = 0.4;
				Color border_color((int)(((double)color.GetR())*darken), (int)(((double)color.GetG())*darken), (int)(((double)color.GetB())*darken));
				AddSeries(profile).Legend(profile_legend).PlotStyle<BarSeriesPlot>().BarWidth(0.5).NoMark().Fill(color).Stroke(1.0, border_color).Units(profile_unit);
			} else if (mode == Plot_Mode::profile2D && !profile2D_is_timed) {
				int idx = 0;
				for(auto &series : series_data)
					profile2D.add(&series);
				AddSurf(profile2D);
				SurfRainbow(BLUE_WHITE_RED);   // TODO: Make this selectable
			} else if ((mode == Plot_Mode::profile2D) && profile2D_is_timed) {
				int dimy = setup.selected_indexes[profile_set_idx].size();
				int dimx = setup.selected_indexes[profile_set_idx2].size();
				
				for(int idx = 0; idx < dimx*dimy; ++idx)
					profile2Dt.add(&series_data[idx]);
				profile2Dt.set_size(dimx, dimy);
				
				//PromptOK(Format("Series data size is %d = %d*%d", series_data.size(), dimx, dimy));
				
				AddSurf(profile2Dt);
				SurfRainbow(WHITE_BLACK);
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
			
			Indexes indexes(parent->model);
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
	
	if(mode == Plot_Mode::profile || (mode == Plot_Mode::profile2D && profile2D_is_timed)) {
		replot_profile();
	} else
		Refresh();
}

void
MyPlot::replot_profile() {
	profile.set_ts(setup.profile_time_step);
	profile2Dt.set_ts(setup.profile_time_step);
	Refresh();
}

void 
serialize_plot_setup(Model_Application *app, Json &json_data, Plot_Setup &setup) {
	
	auto model = app->model;
		
	JsonArray sel_results;
	for(Var_Id var_id : setup.selected_results)
		sel_results << app->serialize(var_id).data();
	json_data("sel_results", sel_results);
	
	JsonArray sel_series;
	for(Var_Id var_id : setup.selected_series)
		sel_series << app->serialize(var_id).data();
	json_data("sel_series", sel_series);
	
	Json index_sets;
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		Entity_Id index_set = {Reg_Type::index_set, idx};
		
		if(idx > model->index_sets.count() || idx >= setup.selected_indexes.size() || setup.selected_indexes[idx].empty()) continue;
		
		JsonArray indexes;
		for(Index_T index : setup.selected_indexes[idx]) {
			if(!is_valid(index)) continue;
			indexes << app->get_index_name(index).data();
		}
		index_sets(model->serialize(index_set).data(), indexes);
	}
	json_data("index_sets", index_sets);

	// TODO: This will break if one saves the string and the enums change in the mean time.
	// Should really save by name.	
	json_data("mode", (int)setup.mode);
	json_data("aggregation_type", (int)setup.aggregation_type);
	json_data("aggregation_period", (int)setup.aggregation_period);
	json_data("y_axis_mode", (int)setup.y_axis_mode);
	json_data("scatter_inputs", setup.scatter_inputs);
	json_data("pivot_month", setup.pivot_month);
	json_data("profile_time_step", setup.profile_time_step);
}

String
serialize_plot_setup(Model_Application *app, Plot_Setup &setup) {
	Json json_data;
	serialize_plot_setup(app, json_data, setup);
	return json_data.ToString();
}


Plot_Setup
deserialize_plot_setup(Model_Application *app, Value &setup_json) {
	
	auto model = app->model;
	
	Plot_Setup setup;
	
	Value mode = setup_json["mode"];
	if(!IsNull(mode)) setup.mode = (Plot_Mode)(int)mode;
	
	Value aggregation_type = setup_json["aggregation_type"];
	if(!IsNull(aggregation_type)) setup.aggregation_type = (Aggregation_Type)(int)aggregation_type;
	
	Value aggregation_period = setup_json["aggregation_period"];
	if(!IsNull(aggregation_period)) setup.aggregation_period = (Aggregation_Period)(int)aggregation_period;
	
	Value y_axis_mode = setup_json["y_axis_mode"];
	if(!IsNull(y_axis_mode)) setup.y_axis_mode = (Y_Axis_Mode)(int)y_axis_mode;
	
	Value scatter_inputs = setup_json["scatter_inputs"];
	if(!IsNull(scatter_inputs)) setup.scatter_inputs = (bool)scatter_inputs;
	
	Value pivot_month = setup_json["pivot_month"];
	if(!IsNull(pivot_month)) setup.pivot_month = (int)pivot_month;
	
	Value profile_time_step = setup_json["profile_time_step"];
	if(!IsNull(profile_time_step)) setup.profile_time_step = (int)profile_time_step;
	
	ValueArray sel_results = setup_json["sel_results"];
	if(!IsNull(sel_results)) {
		int count = sel_results.GetCount();
		for(int idx = 0; idx < count; ++idx) {
			Var_Id var_id = app->deserialize(sel_results[idx].ToStd());
			if(is_valid(var_id) && var_id.type == Var_Id::Type::state_var)
				setup.selected_results.push_back(var_id);
		}
	}
	ValueArray sel_series = setup_json["sel_series"];
	if(!IsNull(sel_series)) {
		int count = sel_series.GetCount();
		for(int idx = 0; idx < count; ++idx) {
			Var_Id var_id = app->deserialize(sel_series[idx].ToStd());
			if(is_valid(var_id) && var_id.type != Var_Id::Type::state_var)
				setup.selected_series.push_back(var_id);
			//else
				//PromptOK(Format("Invalid var_id %d %d for series \"%s\"", (int)var_id.type, var_id.id, sel_series[idx].ToString()));
		}
	}
	
	setup.selected_indexes.resize(MAX_INDEX_SETS);
	setup.index_set_is_active.resize(MAX_INDEX_SETS);
	
	ValueMap index_sets = setup_json["index_sets"];
	if(!IsNull(index_sets)) {
		int count = index_sets.GetCount();
		for(int idx = 0; idx < count; ++idx) {
			auto index_set_id = model->deserialize(index_sets.GetKey(idx).ToStd(), Reg_Type::index_set);
			if(!is_valid(index_set_id)) continue;
			ValueArray indexes = index_sets.GetValue(idx);
			if(!IsNull(indexes)) {
				int count2 = indexes.GetCount();
				for(int idx2 = 0; idx2 < count2; ++idx2) {
					Value index_name = indexes[idx2];
					Index_T index = app->get_index(index_set_id, index_name.ToStd());
					if(is_valid(index))
						setup.selected_indexes[index_set_id.id].push_back(index);
				}
			}
		}
	}
	
	
	register_if_index_set_is_active(setup, app);
	
	return std::move(setup);
}

Plot_Setup
deserialize_plot_setup(Model_Application *app, String &data) {
	Value setup_json  = ParseJSON(data);
	return deserialize_plot_setup(app, setup_json);
}