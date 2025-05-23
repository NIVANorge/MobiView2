#ifndef _MobiView2_PlotCtrl_h_
#define _MobiView2_PlotCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <ScatterCtrl/ScatterCtrl.h>

#include "ParameterCtrl.h"
#include "MyRichView.h"
#include "support/statistics.h"
#include "support/aggregate.h"
#include "model_application.h"

// NOTE: This has to match up to the aggregation selector. It should also match the override
// modes in the AdditionalPlotView
enum class Plot_Mode {
	regular = 0,
	compare_baseline,
	stacked,
	stacked_share,
	histogram,
	profile,
	profile2D,
	baseline2D,
	residuals,
	residuals_histogram,
	qq,
	
	none = 100, // Used to signify a non-override in additional plot view
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
		|| mode==Plot_Mode::baseline2D || mode==Plot_Mode::compare_baseline
		|| mode==Plot_Mode::residuals);
}

struct Plot_Setup {
	Plot_Mode          mode;
	Aggregation_Type   aggregation_type;
	Aggregation_Period aggregation_period;
	Y_Axis_Mode        y_axis_mode;
	bool               scatter_inputs;
	int                pivot_month;
	
	int profile_time_step;
	
	std::vector<Var_Id> selected_results;
	std::vector<Var_Id> selected_series;
	
	std::vector<u8> index_set_is_active;  // NOTE: should be bool, but that has strange behaviour.
	std::vector<std::vector<Index_T>> selected_indexes;
};

void        serialize_plot_setup(Model_Application *app, Upp::Json &json, Plot_Setup &setup);
Upp::String serialize_plot_setup(Model_Application *app, Plot_Setup &setup);

Plot_Setup  deserialize_plot_setup(Model_Application *app, Upp::Value &json);
Plot_Setup  deserialize_plot_setup(Model_Application *app, Upp::String &data);

class Mobius_Data_Source : public Upp::DataSource {
public :
	virtual bool is_empty() { return false; }
};

class Mobius_Series_Data_Source : public Mobius_Data_Source {
public :
	Mobius_Series_Data_Source(Data_Storage<double, Var_Id> *data, s64 offset, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size)
		: data(data), offset(offset), x_data(x_data), steps(steps) {
		
		x_offset = steps_between(ref_x_start, start, ts_size);
		y_offset = steps_between(data->start_date, start, ts_size);
	}
		
	virtual double y(s64 id) { return *data->get_value(offset, y_offset + id); }
	virtual double x(s64 id) { return x_data[x_offset + id]; }
	virtual s64 GetCount() const { return steps; }
	
	virtual bool is_empty() {
		s64 count = GetCount();
		bool some = false;
		for(s64 i = 0; i < count; ++i) {
			if(std::isfinite(y(i))) {
				some = true;
				break;
			}
		}
		return some;
	}
private :
	Data_Storage<double, Var_Id> *data;
	s64 offset, x_offset, y_offset, steps;
	double *x_data;
};

class Residual_Data_Source : public Mobius_Data_Source {
	
public:
	Residual_Data_Source(Data_Storage<double, Var_Id> *data_sim, Data_Storage<double, Var_Id> *data_obs, s64 offset_sim, s64 offset_obs, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size) :
		
		sim(data_sim, offset_sim, steps, x_data, ref_x_start, start, ts_size),
		obs(data_obs, offset_obs, steps, x_data, ref_x_start, start, ts_size)
	{}
	
	virtual double y(s64 id) { return sim.y(id) - obs.y(id); }
	virtual double x(s64 id) { return obs.x(id); }
	virtual s64 GetCount() const { return obs.GetCount(); }
	
private:
	Mobius_Series_Data_Source sim;
	Mobius_Series_Data_Source obs;
};

class Agg_Data_Source : public Mobius_Data_Source {
	
public :
	Agg_Data_Source(Data_Storage<double, Var_Id> *data, s64 offset, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size, Plot_Setup *setup, bool always_copy_y = false)
		: y_axis_mode(setup->y_axis_mode), aggregation_period(setup->aggregation_period) {
		
		source = new Mobius_Series_Data_Source(data, offset, steps, x_data, ref_x_start, start, ts_size);
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
		
		if(y_axis_mode == Y_Axis_Mode::normalized)
			return yval / max;
		//else if(y_axis_mode == Y_Axis_Mode::logarithmic) // This is now automatic
		//	return std::log10(yval);
		return yval;
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
			aggregate_data(ref_x_start, start, source, setup->aggregation_period, setup->aggregation_type, ts_size, setup->pivot_month, agg_x, agg_y);
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
		//TODO: This is not that good. Needs to be more robust.
		if(max == 0.0) max = 1.0;
	}
	
	bool copied_y;
	Upp::DataSource *source;
	double max;
	Y_Axis_Mode y_axis_mode;
	Aggregation_Period aggregation_period;
	std::vector<double> agg_x;
	std::vector<double> agg_y;
};

class Data_Source_Line : public Mobius_Data_Source {

public:
	Data_Source_Line(double x0, double y0, double x1, double y1)
		: xx {x0, x1}, yy {y0, y1} {}
	virtual double x(s64 id) { return xx[id]; }
	virtual double y(s64 id) { return yy[id]; }
	virtual s64 GetCount() const { return 2; }
	
private:
	double xx[2], yy[2];
};

class Data_Source_Owns_XY : public Mobius_Data_Source {
	
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

class Data_Source_Profile : public Mobius_Data_Source {
	
public :
	Data_Source_Profile() : ts(0) { clear(); }
	
	void add(Mobius_Data_Source *val) {
		data.push_back(val);
		for(s64 ts = 0; ts < val->GetCount(); ++ts) {
			double v = val->y(ts);
			if(std::isfinite(v)) {
				mmax = std::max(mmax, v);
				mmin = std::min(mmin, v);
			}
		}
	}
	void set_ts(s64 _ts) { ts = _ts; }
	void clear() {
		data.clear();
		x_values.clear();
		ts = 0;
		mmin = std::numeric_limits<double>::infinity();
		mmax = -mmin;
	}
	double get_max() { return mmax; }
	double get_min() { return mmin; }
	void set_x_values(std::vector<double> &x_values) { this->x_values = x_values; }
	bool has_x_values() { return !x_values.empty(); }
	
	virtual double x(s64 id) {
		if(!x_values.empty()) return x_values[id]+0.5;
		return (double)id;
	}
	virtual double y(s64 id) { return data[id]->y(ts); }
	virtual s64 GetCount() const { return (s64)data.size(); }
	
private:
	s64 ts;
	double mmin, mmax;
	std::vector<Upp::DataSource *> data;
	std::vector<double> x_values;
};

class Table_Data_Profile2D : public Upp::TableData {

public:
	Table_Data_Profile2D() {
		inter = TableInterpolate::NO;
		areas = true;
		rowMajor = true;
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
	void clear() { sources.clear(); y_values.clear(); }
	s64 count() { return sources.size(); }
	
	void set_y_values(std::vector<double> &y_values_in) {
		y_values.resize(y_values_in.size());
		//log_print("Got y values:\n");
		for(int id = 0; id < y_values.size(); ++id) {
			y_values[id] = -y_values_in[(int)y_values.size() - id - 1];
			//log_print(y_values[id], " ");
		}
		//log_print("\n");
	}
	bool has_y_values() { return !y_values.empty(); }
	
	virtual double x(int id) { return sources[0]->x((s64)id); }
	virtual double y(int id) {
		if(!y_values.empty()) return y_values[id];
		return (double)id;
	}
	virtual double zdata(int id) {
		s64 yy = (s64)id / (s64)(lenxAxis-1);
		s64 xx = ((s64)id) % ((s64)lenxAxis-1);
		return sources[yy]->y(xx);
	}
	
private:
	std::vector<Upp::DataSource *> sources;
	std::vector<double>            y_values;
};

class Table_Data_Profile2DTimed : public Upp::TableData {

public:
	Table_Data_Profile2DTimed() : ts(0) {
		inter = TableInterpolate::NO;
		areas = true;
		rowMajor = true;
		clear();
	}
	void clear() { sources.clear(); min = std::numeric_limits<double>::infinity(); max = -min; set_size(0, 0); }
	s64 count() { return sources.size(); }
	void set_ts(s64 _ts) { ts = _ts; }
	void set_size(s64 dimx, s64 dimy) {
		this->lenxAxis = dimx+1;
		this->lenyAxis = dimy+1;
		this->lendata = dimx*dimy;
	}
	
	void add(Upp::DataSource *val) {
		sources.push_back(val);
		for(s64 ts = 0; ts < val->GetCount(); ++ts) {
			double v = val->y(ts);
			max = std::max(max, v);
			min = std::min(min, v);
		}
	}
	double get_max() { return max; }
	double get_min() { return min; }
	double get_dim_x() { return lenxAxis-1; }
	double get_dim_y() { return lenyAxis-1; }
	
	virtual double x(int id) { return (double)id; }
	virtual double y(int id) { return (double)id; }
	virtual double zdata(int id) {
		return sources[id]->y(ts);
	}
	
private:
	double min, max;
	s64 ts;
	std::vector<Upp::DataSource *> sources;
};

class Table_Data_Owns_XYZ : public Upp::TableData {

public :
	Table_Data_Owns_XYZ(s64 dim_x, s64 dim_y) {
		inter = TableInterpolate::NO;
		areas = true;
		rowMajor = true;
		xx.resize(dim_x);
		yy.resize(dim_y);
		lenxAxis = dim_x;
		lenyAxis = dim_y;
		lendata = (dim_x - 1)*(dim_y - 1);
		zz.resize(lendata);
	}
	
	virtual double x(int id) { return xx[id]; }
	virtual double y(int id) { return yy[id]; }
	virtual double zdata(int id) { return zz[id]; }
	
	double &set_x(s64 idx) { return xx[idx]; }
	double &set_y(s64 idx) { return yy[idx]; }
	double &set_z(s64 x_idx, s64 y_idx) {
		return zz[x_idx + (lenxAxis-1)*y_idx];
	}
	void clear_z() { for(int idx = 0; idx < zz.size(); ++idx) zz[idx] = 0.0; }
private :
	std::vector<double> xx, yy, zz;
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
			else          res += each_data[i].real_y(id);
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
	
	Upp::Array<Mobius_Data_Source> series_data;
	std::vector<double>         x_data;
	MyDataStackedY              data_stacked;
	Upp::Histogram              histogram;
	Data_Source_Profile         profile;
	Table_Data_Profile2D        profile2D;
	Table_Data_Profile2DTimed   profile2Dt;
	bool profile2D_is_timed = false;
	
	Plot_Colors                 colors;
	Residual_Stats              cached_stats;
	
	Upp::Vector<Upp::String>    labels, labels2;
	Upp::String                 profile_legend, profile_unit;
};


#define LAYOUTFILE <MobiView2/PlotCtrl.lay>
#include <CtrlCore/lay.h>

class MobiView2;

class PlotCtrl : public WithPlotCtrlLayout<Upp::ParentCtrl> {
public:
	typedef PlotCtrl CLASSNAME;
	
	PlotCtrl(MobiView2 *parent);
	
	void index_selection_change(Entity_Id id = invalid_entity_id, bool replot = true);
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
	
	
	Upp::Array<Upp::ArrayCtrl>    index_lists;
	Upp::Array<Upp::ButtonOption> push_sel_alls;
	
	Date_Time profile_base_time;
	
	MobiView2 *parent;
	
private :
	bool index_callback_lock = false;
};

void register_if_index_set_is_active(Plot_Setup &ps, Model_Application *app);

//void get_storage_and_var(Model_Data *md, Var_Id var_id, Data_Storage<double, Var_Id> **data, State_Var **var);

bool add_single_plot(MyPlot *draw, Model_Data *md, Model_Application *app, Var_Id var_id, Indexes &indexes,
	s64 ts, Date_Time ref_x_start, Date_Time start, double *x_data, s64 gof_offset, s64 gof_ts, Upp::Color &color, bool stacked = false,
	const Upp::String &legend_prefix = Upp::Null, bool always_copy_y = false);
	
int add_histogram(MyPlot *plot, Upp::DataSource *data, double min, double max, s64 count, const Upp::String &legend, const Upp::String &unit, const Upp::Color &color);
	
void get_single_indexes(Model_Application *app, Indexes &indexes, Plot_Setup &setup);

void get_gof_offsets(Upp::Time &start_setting, Upp::Time &end_setting, Date_Time input_start, s64 input_ts, Date_Time result_start, s64 result_ts, Date_Time &gof_start, Date_Time &gof_end,
	s64 &input_gof_offset, s64 &result_gof_offset, s64 &gof_ts, Time_Step_Size ts_size, bool has_results);

void set_round_grid_line_positions(Upp::ScatterDraw *plot, int axis);

void format_axes(MyPlot *plot, Plot_Mode mode, int n_bins_histogram, Date_Time input_start, Time_Step_Size ts_size);

void add_line(MyPlot *plot, double x0, double y0, double x1, double y1, Upp::Color color, const Upp::String &legend = Upp::Null);


#endif
