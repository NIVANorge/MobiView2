#ifndef _MobiView2_MobiView2_h
#define _MobiView2_MobiView2_h

#include <CtrlLib/CtrlLib.h>


#include <ScatterCtrl/ScatterCtrl.h>
#include <AutoScroller/AutoScroller.h>

#include <map>
#include <vector>

#include "StarOption.h"
#include "MyRichView.h"


//#include "MCMC.h"

//class MobiView2;

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

/*
class ChangeIndexesWindow;

class VisualizeBranches : public ParentCtrl
{
public :
	VisualizeBranches();
	
	MobiView *ParentWindow;
	ChangeIndexesWindow *OtherParent;
	
	virtual void Paint(Draw &W);
};

class ChangeIndexesWindow : public WithChangeIndexesLayout<TopWindow>
{
public:
	typedef ChangeIndexesWindow CLASSNAME;
	
	ChangeIndexesWindow();
	
	MobiView *ParentWindow;
	
	VisualizeBranches Branches;
	
	Label     *IndexSetName[MAX_INDEX_SETS];
	LineEdit  *IndexList[MAX_INDEX_SETS];
	ArrayCtrl *BranchList[MAX_INDEX_SETS];
	
	Array<Array<Ctrl>> BranchControls;
	Array<Array<Ctrl>> NameControls;
	
	bool ParseIntList(String &ListStr, std::vector<int> &Result, int Row);
	
	void RefreshData();
	void DoIndexUpdate();
	void AddIndexPushed();
	void DeleteSelectedPushed();
	
	void SelectedBranchListUpdate();
	
	void BuildConnectionEditFromDataset();
};


class VarianceSensitivityWindow : public WithVarSensitivityResultLayout<TopWindow>
{
public :
	typedef VarianceSensitivityWindow CLASSNAME;
	
	VarianceSensitivityWindow();
};
*/


#include "model_application.h" //TODO: may not need to include this entire file..

#include "Util.h"


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

	//Upp::ParentCtrl result_selecter_rect;
	//Upp::TreeCtrl   result_selecter;
	//Upp::Option     show_favorites;
	//Upp::TreeCtrl   input_selecter;
	
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
	StructureViewWindow   structure_window;
	
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
	
	void load();
	void reload(bool recompile_only = false);
	void save_parameters();
	void save_parameters_as();
	void run_model();
	
	void save_baseline();
	void revert_baseline();
	
	//void SaveInputsAsDat();
	
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
	
private :
	void delete_model();
	bool do_the_load();
};



#endif