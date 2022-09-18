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

/*
TODO: should maybe have some composable classes
Mobius_Base_Data_Source
(2 x Mobius_Base_Data_Source) -> Residual_Data_Source
(Mobius_Base_Data_Source -> Agg_Data_Source)
*/

class Mobius_Data_Source : public Upp::DataSource {
public :
	Mobius_Data_Source(Structured_Storage<double, Var_Id> *data, s64 offset, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size)
		: data(data), offset(offset), x_data(x_data), steps(steps) {
		
		x_offset = steps_between(ref_x_start, start, ts_size);
		y_offset = steps_between(data->start_date, start, ts_size);
	}
		
	virtual double y(s64 id) { return *data->get_value(offset, y_offset + id); }
	virtual double x(s64 id) { return x_data[x_offset + id]; }
	virtual s64 GetCount() const { return steps; }
private :
	Structured_Storage<double, Var_Id> *data;
	s64 offset, x_offset, y_offset, steps;
	double *x_data;
};

class Residual_Data_Source : public Upp::DataSource {
	
public:
	Residual_Data_Source(Structured_Storage<double, Var_Id> *data_sim, Structured_Storage<double, Var_Id> *data_obs, s64 offset_sim, s64 offset_obs, s64 steps, double *x_data,
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
	Agg_Data_Source(Structured_Storage<double, Var_Id> *data, s64 offset, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size, Plot_Setup *setup)
		: y_axis_mode(setup->y_axis_mode), aggregation_period(setup->aggregation_period) {
		
		source = new Mobius_Data_Source(data, offset, steps, x_data, ref_x_start, start, ts_size);
		build(ref_x_start, start, setup, steps, ts_size);
	}
	
	Agg_Data_Source(Structured_Storage<double, Var_Id> *data_sim, Structured_Storage<double, Var_Id> *data_obs, s64 offset_sim, s64 offset_obs, s64 steps, double *x_data,
		Date_Time ref_x_start, Date_Time start, Time_Step_Size ts_size, Plot_Setup *setup)
		: y_axis_mode(setup->y_axis_mode), aggregation_period(setup->aggregation_period) {
		
		source = new Residual_Data_Source(data_sim, data_obs, offset_sim, offset_obs, steps, x_data, ref_x_start, start, ts_size);
		build(ref_x_start, start, setup, steps, ts_size);
	}
	
	inline double get_actual_y(s64 id) {
		if(aggregation_period == Aggregation_Period::none)
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
		if(aggregation_period == Aggregation_Period::none)
			return source->GetCount();
		return agg_x.size();
	}
	
	~Agg_Data_Source() { delete source; }
private :
	inline void build(Date_Time ref_x_start, Date_Time start, Plot_Setup *setup, s64 steps, Time_Step_Size ts_size) {
		if(aggregation_period != Aggregation_Period::none)
			aggregate_data(ref_x_start, start, source, setup->aggregation_period, setup->aggregation_type, ts_size, agg_x, agg_y);

		max = -std::numeric_limits<double>::infinity();
		if(y_axis_mode == Y_Axis_Mode::normalized) {
			for(s64 step = 0; step < steps; ++step) {
				double yy = get_actual_y(step);
				if(std::isfinite(yy))
					max = std::max(max, yy);
			}
		}
	}
	
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
		xx(std::move(*xx)), yy(std::move(*yy)) {}
	virtual double x(s64 id) { return xx[id]; }
	virtual double y(s64 id) { return yy[id]; }
	virtual s64 GetCount() const { return (s64)xx.size(); }
	
private:
	std::vector<double> xx, yy;
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
	
	//EachDataStackedY &get(int id) { return each_data[id]; }
	EachDataStackedY &top()       { return each_data.Top(); }
	void clear() { each_data.Clear(); }

protected:
	Upp::Array<EachDataStackedY> each_data;
	bool is_share;
};

class MyPlot : public Upp::ScatterCtrl {
public:
	typedef MyPlot CLASSNAME;
	
	MyPlot();
	
	void clean(bool full_clean = true);
	void build_plot(bool cause_by_run = false, Plot_Mode override_mode = Plot_Mode::none);
	
	Plot_Setup setup;
	
	MyRichView *plot_info;
	MobiView2  *parent;
	
	bool was_auto_resized = false;
	
	Upp::Array<Upp::DataSource> series_data;
	std::vector<double> x_data;
	MyDataStackedY data_stacked;
	Upp::Histogram histogram;
	
	Plot_Colors colors;
	Residual_Stats cached_stats;
	
	Upp::Vector<Upp::String> qq_labels;
	
private:
	void compute_x_data(Date_Time start, s64 steps, Time_Step_Size ts_size);
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
	
	void build_time_intervals_ctrl();
	
	void get_plot_setup(Plot_Setup &plot_setup);
	void set_plot_setup(Plot_Setup &plot_setup);
	
	
	void clean();
	void build_index_set_selecters(Model_Application *app);
	
	std::vector<Entity_Id> index_sets;
	Upp::ArrayCtrl *index_list[MAX_INDEX_SETS];
	
	//Upp::Time profile_display_time;
	
	MobiView2 *parent;
};
#endif
