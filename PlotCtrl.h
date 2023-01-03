#ifndef _MobiView2_PlotCtrl_h_
#define _MobiView2_PlotCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <ScatterCtrl/ScatterCtrl.h>

#include "ParameterCtrl.h"
#include "MyRichView.h"
#include "support/statistics.h"
#include "model_application.h"

//NOTE: This has to match up to the aggregation selector. It should also match the override
//modes in the AdditionalPlotView
enum class Plot_Mode {
	regular = 0,
	stacked,
	stacked_share,
	histogram,
	profile,
	profile2D,
	compare_baseline,
	residuals,
	residuals_histogram,
	qq,
	
	none = 100, // Only used to signify a non-override
};

//NOTE: This has to match up to the aggregation selector.
enum class Aggregation_Type {
	mean = 0,
	sum,
	min,
	max,
};

//NOTE: The matching of this with the selector should be dynamic, so no worries.
enum class Aggregation_Period {
	none = 0,
	weekly,
	monthly,
	yearly,
};

//NOTE: This has to match up to the y axis mode selector.
enum class Y_Axis_Mode {
	regular = 0,
	normalized,
	logarithmic,
};

// denotes whether or not it makes sense to link the x axis of this one to other plots.
inline bool is_linkable(Plot_Mode mode) {
	return (mode==Plot_Mode::regular || mode==Plot_Mode::stacked
		|| mode==Plot_Mode::stacked_share || mode==Plot_Mode::profile2D
		|| mode==Plot_Mode::compare_baseline || mode==Plot_Mode::residuals);
}

struct Plot_Setup {
	Plot_Mode          mode;
	Aggregation_Type   aggregation_type;
	Aggregation_Period aggregation_period;
	Y_Axis_Mode        y_axis_mode;
	bool               scatter_inputs;
	
	std::vector<Var_Id> selected_results;
	std::vector<Var_Id> selected_series;
	
	std::vector<u8> index_set_is_active;  // NOTE: should be bool, but that has strange behaviour.
	std::vector<std::vector<Index_T>> selected_indexes;
	
	int profile_time_step;
};

class Mobius_Data_Source : public Upp::DataSource {
public :
	Mobius_Data_Source(Data_Storage<double, Var_Id> *data, s64 offset, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size)
		: data(data), offset(offset), x_data(x_data), steps(steps) {
		
		x_offset = steps_between(ref_x_start, start, ts_size);
		y_offset = steps_between(data->start_date, start, ts_size);
	}
		
	virtual double y(s64 id) { return *data->get_value(offset, y_offset + id); }
	virtual double x(s64 id) { return x_data[x_offset + id]; }
	virtual s64 GetCount() const { return steps; }
private :
	Data_Storage<double, Var_Id> *data;
	s64 offset, x_offset, y_offset, steps;
	double *x_data;
};

class Residual_Data_Source : public Upp::DataSource {
	
public:
	Residual_Data_Source(Data_Storage<double, Var_Id> *data_sim, Data_Storage<double, Var_Id> *data_obs, s64 offset_sim, s64 offset_obs, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size) :
		
		sim(data_sim, offset_sim, steps, x_data, ref_x_start, start, ts_size),
		obs(data_obs, offset_obs, steps, x_data, ref_x_start, start, ts_size)
	{}
	
	virtual double y(s64 id) { return obs.y(id) - sim.y(id); }
	virtual double x(s64 id) { return obs.x(id); }
	virtual s64 GetCount() const { return obs.GetCount(); }
	
private:
	Mobius_Data_Source sim;
	Mobius_Data_Source obs;
};

void
aggregate_data(Date_Time &ref_time, Date_Time &start_time, Upp::DataSource *source,
	Aggregation_Period agg_period, Aggregation_Type agg_type, Time_Step_Size ts_size, std::vector<double> &x_vals, std::vector<double> &y_vals);

class Agg_Data_Source : public Upp::DataSource {
	
public :
	Agg_Data_Source(Data_Storage<double, Var_Id> *data, s64 offset, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size, Plot_Setup *setup, bool always_copy_y = false)
		: y_axis_mode(setup->y_axis_mode), aggregation_period(setup->aggregation_period) {
		
		source = new Mobius_Data_Source(data, offset, steps, x_data, ref_x_start, start, ts_size);
		copied_y = always_copy_y;
		build(ref_x_start, start, setup, steps, ts_size);
	}
	
	Agg_Data_Source(Data_Storage<double, Var_Id> *data_sim, Data_Storage<double, Var_Id> *data_obs, s64 offset_sim, s64 offset_obs, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size, Plot_Setup *setup)
		: y_axis_mode(setup->y_axis_mode), aggregation_period(setup->aggregation_period) {
		
		source = new Residual_Data_Source(data_sim, data_obs, offset_sim, offset_obs, steps, x_data, ref_x_start, start, ts_size);
		copied_y = false;
		build(ref_x_start, start, setup, steps, ts_size);
	}
	
	inline double get_actual_y(s64 id) {
		if(!copied_y)
			return source->y(id);
		return agg_y[id];
	}
		
	virtual double y(s64 id) {
		double yval = get_actual_y(id);
		if(y_axis_mode == Y_Axis_Mode::regular)
			return yval;
		if(y_axis_mode == Y_Axis_Mode::normalized)
			return yval / max;
		else
			return std::log10(yval);
	}
	
	virtual double x(s64 id) {
		if(aggregation_period == Aggregation_Period::none)
			return source->x(id);
		return agg_x[id];
	}
	
	virtual s64 GetCount() const {
		if(!copied_y)
			return source->GetCount();
		return agg_y.size();
	}
	
	~Agg_Data_Source() { delete source; }
private :
	inline void build(Date_Time ref_x_start, Date_Time start, Plot_Setup *setup, s64 steps, Time_Step_Size ts_size) {
		
		if(aggregation_period == Aggregation_Period::none) {
			if(copied_y) {
				agg_y.resize(steps);
				for(s64 ts = 0; ts < steps; ++ts) agg_y[ts] = source->y(ts);
			}
		} else {
			aggregate_data(ref_x_start, start, source, setup->aggregation_period, setup->aggregation_type, ts_size, agg_x, agg_y);
			copied_y = true;
		}

		max = -std::numeric_limits<double>::infinity();
		if(y_axis_mode == Y_Axis_Mode::normalized) {
			for(s64 step = 0; step < steps; ++step) {
				double yy = get_actual_y(step);
				if(std::isfinite(yy))
					max = std::max(max, yy);
			}
		}
	}
	
	bool copied_y;
	Upp::DataSource *source;
	double max;
	Y_Axis_Mode y_axis_mode;
	Aggregation_Period aggregation_period;
	std::vector<double> agg_x;
	std::vector<double> agg_y;
};

class Data_Source_Line : public Upp::DataSource {

public:
	Data_Source_Line(double x0, double y0, double x1, double y1)
		: xx {x0, x1}, yy {y0, y1} {}
	virtual double x(s64 id) { return xx[id]; }
	virtual double y(s64 id) { return yy[id]; }
	virtual s64 GetCount() const { return 2; }
	
private:
	double xx[2], yy[2];
};

class Data_Source_Owns_XY : public Upp::DataSource {
	
public:
	Data_Source_Owns_XY(std::vector<double> *xx, std::vector<double> *yy) :
		xx(*xx), yy(*yy) {}                                   // NOTE: previously had std::move on these, but found out we need to reuse some vectors for multiple series
	Data_Source_Owns_XY(s64 dim_x, s64 dim_y) :
		xx(dim_x), yy(dim_y) {}
		
	virtual double x(s64 id) { return xx[id]; }
	virtual double y(s64 id) { return yy[id]; }
	virtual s64 GetCount() const { return (s64)xx.size(); }
	
	double &set_x(int id) { return xx[id]; }
	double &set_y(int id) { return yy[id]; }
private:
	std::vector<double> xx, yy;
};

class Data_Source_Profile : public Upp::DataSource {
	
public :
	Data_Source_Profile() : ts(0), max(-std::numeric_limits<double>::infinity()) {}
	
	void add(Upp::DataSource *val) {
		data.push_back(val);
		for(s64 ts = 0; ts < val->GetCount(); ++ts) {
			max = std::max(max, val->y(ts));
			min = std::min(min, val->y(ts));
		}
	}
	void set_ts(s64 _ts) { ts = _ts; }
	void clear() { data.clear(); ts = 0; min = std::numeric_limits<double>::infinity(); max = -min; }
	double get_max() { return max; }
	double get_min() { return min; }
	
	virtual double x(s64 id) { return (double)id + 0.5; }
	virtual double y(s64 id) { return data[id]->y(ts); }
	virtual s64 GetCount() const { return (s64)data.size(); }
	
private:
	s64 ts;
	double min, max;
	std::vector<Upp::DataSource *> data;
};

class Table_Data_Profile2D : public Upp::TableData {

public:
	//TODO: should switch this to use areas
	Table_Data_Profile2D() {
		inter = TableInterpolate::NO;
		areas = true;
	}
	
	void add(Upp::DataSource *val) {
		sources.insert(sources.begin(), val);
		//TODO: ideally this should be +1, but then we would have to instruct the sources to
		//provide 1 more x value, which is tricky the way the infrastructure is set up now.
		// this means that now the last time step value is not displayed.
		this->lenxAxis = val->GetCount();
		this->lenyAxis = sources.size()+1;
		this->lendata = (lenxAxis-1)*(lenyAxis-1);
	}
	void clear() { sources.clear(); }
	s64 count() { return sources.size(); }
	
	virtual double x(int id) { return sources[0]->x((s64)id); }
	virtual double y(int id) { return (double)id; }
	virtual double data(int id) {
		s64 yy = (s64)id / (s64)(lenxAxis-1);
		s64 xx = ((s64)id) % ((s64)lenxAxis-1);
		return sources[yy]->y(xx);
	}
	
private:
	std::vector<Upp::DataSource *> sources;
};

class Table_Data_Owns_XYZ : public Upp::TableData {

public :
	Table_Data_Owns_XYZ(s64 dim_x, s64 dim_y) {
		inter = TableInterpolate::NO;
		areas = true;
		xx.resize(dim_x);
		yy.resize(dim_y);
		lenxAxis = dim_x;
		lenyAxis = dim_y;
		lendata = (dim_x - 1)*(dim_y - 1);
		zz.resize(lendata);
	}
	
	virtual double x(int id) { return xx[id]; }
	virtual double y(int id) { return yy[id]; }
	virtual double data(int id) { return zz[id]; }
	
	double &set_x(s64 idx) { return xx[idx]; }
	double &set_y(s64 idx) { return yy[idx]; }
	double &set_z(s64 x_idx, s64 y_idx) {
		return zz[x_idx + (lenxAxis-1)*y_idx];
	}
	void clear_z() { for(int idx = 0; idx < zz.size(); ++idx) zz[idx] = 0.0; }
private :
	std::vector<double> xx;
	std::vector<double> yy;
	std::vector<double> zz;
};

//NOTE: a better version of the DataStackedY class where we don't have to add back the plots in reverse
//order.
class MyDataStackedY {
public:
	MyDataStackedY() : is_share(false) {}
	void set_share(bool _is_share)	  { is_share = _is_share; }
	MyDataStackedY &Add(Upp::DataSource &data) {
		EachDataStackedY &each = each_data.Add();
		each.Init(data, each_data.GetCount() - 1, this);
		return *this;
	}
	double get_share_y(int index, s64 id) {
		double acc = 0;
		for (int i = 0; i < each_data.GetCount(); ++i)
			acc += each_data[i].real_y(id);
		if (acc == 0)
			return 0;
		else
			return each_data[index].real_y(id)/acc;
	}
	double get_y(int index, s64 id) {
		double res = 0;
		for(int i = each_data.GetCount()-1; i >= index; --i) {
			if (is_share) res += get_share_y(i, id);
			else       res += each_data[i].real_y(id);
		}
		return res;
	}
	
	class EachDataStackedY : public Upp::DataSource {
	public:
		EachDataStackedY() {}
		void Init(Upp::DataSource &_data, int _index, MyDataStackedY *_parent) {
			ASSERT(!_data.IsExplicit() && !_data.IsParam());
			this->data = &_data;
			this->index = _index;
			this->parent = _parent;
		}
		virtual inline double y(s64 id) { return parent->get_y(index, id); }
		double real_y(s64 id) { return data->y(id); }
		virtual inline double x(s64 id) { return data->x(id);	}
		virtual s64 GetCount() const { return data->GetCount(); }
	private:
		Upp::DataSource *data = 0;
		int index = -1;
		MyDataStackedY *parent = 0;
	};
	
	EachDataStackedY &top()       { return each_data.Top(); }
	void clear() { each_data.Clear(); }

protected:
	Upp::Array<EachDataStackedY> each_data;
	bool is_share;
};

struct Plot_Colors {
	Plot_Colors() : next_idx(0) {}
	
	int next_idx;
	static std::vector<Upp::Color> colors;
	
	Upp::Color next() {
		Upp::Color &result = colors[next_idx++];
		if(next_idx == colors.size()) next_idx = 0;
		return result;
	}
	
	void reset() { next_idx = 0; }
};

class PlotCtrl;

class MyPlot : public Upp::ScatterCtrl {
public:
	typedef MyPlot CLASSNAME;
	
	MyPlot();
	
	void clean(bool full_clean = true);
	void build_plot(bool cause_by_run = false, Plot_Mode override_mode = Plot_Mode::none);
	void replot_profile();
	void compute_x_data(Date_Time start, s64 steps, Time_Step_Size ts_size);
	
	Plot_Setup setup;
	
	MyRichView *plot_info = nullptr;
	MobiView2  *parent;
	PlotCtrl   *plot_ctrl = nullptr;
	
	bool was_auto_resized = false;
	
	Upp::Array<Upp::DataSource> series_data;
	std::vector<double>         x_data;
	MyDataStackedY              data_stacked;
	Upp::Histogram              histogram;
	Data_Source_Profile         profile;
	Table_Data_Profile2D        profile2D;
	
	Plot_Colors                 colors;
	Residual_Stats              cached_stats;
	
	Upp::Vector<Upp::String>    labels;
	Upp::String                 profile_legend, profile_unit;
};


#define LAYOUTFILE <MobiView2/PlotCtrl.lay>
#include <CtrlCore/lay.h>

class MobiView2;

class PlotCtrl : public WithPlotCtrlLayout<Upp::ParentCtrl> {
public:
	typedef PlotCtrl CLASSNAME;
	
	PlotCtrl(MobiView2 *parent);
	
	void plot_change();
	void re_plot(bool caused_by_run = false);
	
	void time_step_slider_event();
	void time_step_edit_event();
	void play_pushed();
	bool is_playing = false;
	
	void build_time_intervals_ctrl();
	
	void get_plot_setup(Plot_Setup &plot_setup);
	void set_plot_setup(Plot_Setup &plot_setup);
	
	
	void clean();
	void build_index_set_selecters(Model_Application *app);
	
	std::vector<Entity_Id> index_sets;
	Upp::ArrayCtrl *index_list[MAX_INDEX_SETS];
	
	Date_Time profile_base_time;
	
	MobiView2 *parent;
};

void register_if_index_set_is_active(Plot_Setup &ps, Model_Application *app);

void get_storage_and_var(Model_Data *md, Var_Id var_id, Data_Storage<double, Var_Id> **data, State_Var **var);

void add_single_plot(MyPlot *draw, Model_Data *md, Model_Application *app, Var_Id var_id, std::vector<Index_T> &indexes,
	s64 ts, Date_Time ref_x_start, Date_Time start, double *x_data, s64 gof_offset, s64 gof_ts, Upp::Color &color, bool stacked = false,
	const Upp::String &legend_prefix = Upp::Null, bool always_copy_y = false);
	
int add_histogram(MyPlot *plot, Upp::DataSource *data, double min, double max, s64 count, const Upp::String &legend, const Upp::String &unit, const Upp::Color &color);
	
void get_single_indexes(std::vector<Index_T> &indexes, Plot_Setup &setup);

void get_gof_offsets(Upp::Time &start_setting, Upp::Time &end_setting, Date_Time input_start, s64 input_ts, Date_Time result_start, s64 result_ts, Date_Time &gof_start, Date_Time &gof_end,
	s64 &input_gof_offset, s64 &result_gof_offset, s64 &gof_ts, Time_Step_Size ts_size, bool has_results);

void set_round_grid_line_positions(Upp::ScatterDraw *plot, int axis);

void format_axes(MyPlot *plot, Plot_Mode mode, int n_bins_histogram, Date_Time input_start, Time_Step_Size ts_size);

void add_line(MyPlot *plot, double x0, double y0, double x1, double y1, Upp::Color color, const Upp::String &legend = Upp::Null);


#endif
