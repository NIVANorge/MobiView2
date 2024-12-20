#ifndef _MobiView2_MobiView2_h
#define _MobiView2_MobiView2_h

#include <corecrt.h>

#include <CtrlLib/CtrlLib.h>


#include <ScatterCtrl/ScatterCtrl.h>
#include <AutoScroller/AutoScroller.h>

#include <map>
#include <vector>

#ifdef _DEBUG
	#define CATCH_ERRORS 0
#else
	#define CATCH_ERRORS 1
#endif


#include "StarOption.h"
#include "MyRichView.h"
#include "SeriesSelecter.h"
#include "ParameterCtrl.h"
#include "PlotCtrl.h"
#include "Statistics.h"
#include "SearchWindow.h"
#include "SensitivityViewWindow.h"
#include "ModelInfoWindow.h"
#include "AdditionalPlotView.h"
#include "OptimizationWindow.h"
#include "StructureView.h"
#include "ModelChartView.h"
#include "Util.h"

#include "model_application.h"


class MobiView2 : public Upp::TopWindow
{
	
public:
	typedef MobiView2 CLASSNAME;
	MobiView2();
	
	Upp::Splitter main_vertical;
	Upp::Splitter upper_horizontal;
	Upp::Splitter lower_horizontal;
	
	Upp::TreeCtrl par_group_selecter;
	ParameterCtrl params;
	
	Array<Entity_Node> par_group_nodes;
	
	Upp::ParentCtrl plot_info_rect;
	Upp::EditTime   calib_start;
	Upp::EditTime   calib_end;
	Upp::Label      calib_label;
	Upp::Option     gof_option;
	
	MyRichView plot_info;
	MyRichView log_box;
	
	SeriesSelecter     result_selecter;
	SeriesSelecter     input_selecter;
	
	Upp::ToolBar    tool_bar;
	
	PlotCtrl        plotter;

	EditStatSettings      stat_settings;
	SearchWindow          search_window;
	SensitivityViewWindow sensitivity_window;
	ModelInfoWindow       info_window;
	AdditionalPlotView    additional_plots;
	OptimizationWindow    optimization_window;
	MCMCResultWindow      mcmc_window;
	VarianceSensitivityWindow variance_window;
	StructureViewWindow   structure_window;
	ModelChartView        model_chart_window;
	
	Mobius_Config      mobius_config;
	bool config_is_loaded = false;
	
	Mobius_Model      *model = nullptr;
	Data_Set          *data_set = nullptr;
	Model_Application *app = nullptr;
	
	Model_Data *baseline = nullptr;
	bool baseline_was_just_saved = false;
	
	std::string model_file;
	std::string data_file;
	

	void sub_bar(Upp::Bar &bar);
	
	void log(Upp::String msg, bool error = false);
	void log_warnings_and_errors();
	
	void load_config();
	void load();
	void reload(bool recompile_only = false);
	void save_parameters();
	void save_parameters_as();
	void run_model();
	
	void save_baseline();
	void revert_baseline();
	
	void clean_interface();
	void build_interface();
	void closing_checks();
	
	void store_settings(bool store_favorites = true);
	void plot_rebuild();
	bool model_is_loaded() { return app && model; }
	
	void open_stat_settings();
	void open_search_window();
	void open_sensitivity_window();
	void open_info_window();
	void open_additional_plots();
	void open_optimization_window();
	void open_structure_view();
	void open_view_chart();
	
	bool select_par_group(Entity_Id group_id);
	Entity_Id get_selected_par_group();
	
private :
	void delete_model();
	bool do_the_load();
};



#endif