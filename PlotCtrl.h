#ifndef _MobiView2_PlotCtrl_h_
#define _MobiView2_PlotCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <ScatterCtrl/ScatterCtrl.h>

#include "ParameterCtrl.h"
#include "common_types.h"

//NOTE: This has to match up to the aggregation selector. It should also match the override
//modes in the AdditionalPlotView
enum class Plot_Major_Mode {
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
	Plot_Major_Mode    major_mode;
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

class MyPlot : public Upp::ScatterCtrl {
public:
	typedef MyPlot CLASSNAME;
	
	void clean();
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
