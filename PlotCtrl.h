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

class Mobius_Data_Source : public Upp::DataSource {
public :
	Mobius_Data_Source(Structured_Storage<double, Var_Id> *data, s64 offset, s64 x_offset, s64 y_offset, s64 steps, double *x_data)
		: data(data), offset(offset), x_offset(x_offset), y_offset(y_offset), x_data(x_data), steps(steps) {}
		
	virtual double y(s64 id) {
		return *data->get_value(offset, y_offset + id);
	}
	
	virtual double x(s64 id) {
		return x_data[x_offset + id];
	}
	
	virtual s64 GetCount() const { return steps; }
private :
	Structured_Storage<double, Var_Id> *data;
	s64 offset;
	s64 x_offset, y_offset;
	s64 steps;
	double *x_data;
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
	
	Upp::Array<Mobius_Data_Source> series_data;
	std::vector<double> x_data;
	MyDataStackedY data_stacked;
	
	Plot_Colors colors;
	Residual_Stats cached_stats;
	
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
