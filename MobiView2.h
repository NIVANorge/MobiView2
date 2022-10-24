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

#include "ParameterCtrl.h"
#include "PlotCtrl.h"
#include "Statistics.h"
#include "SearchWindow.h"
#include "SensitivityViewWindow.h"
#include "ModelInfoWindow.h"
#include "AdditionalPlotView.h"
#include "OptimizationWindow.h"

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

class StructureViewWindow : public WithStructureViewLayout<TopWindow>
{
public:
	typedef StructureViewWindow CLASSNAME;
	
	StructureViewWindow();
	
	MobiView *ParentWindow;
	
	void RefreshText();
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

target_stat_class
GetStatClass(optimization_target &Target);

inline bool operator==(const optimization_target &T1, const optimization_target &T2)
{
	return
		   T1.ResultName    == T2.ResultName
		&& T1.ResultIndexes == T2.ResultIndexes
		&& T1.InputName     == T2.InputName
		&& T1.InputIndexes  == T2.InputIndexes
		&& T1.ErrParSym     == T2.ErrParSym
		&& T1.ErrParNum     == T2.ErrParNum
		&& T1.ResidualStat  == T2.ResidualStat
		&& T1.Stat          == T2.Stat
		&& T1.ErrStruct     == T2.ErrStruct
		&& T1.Weight        == T2.Weight
		&& T1.Begin         == T2.Begin
		&& T1.End           == T2.End;
}

struct triangle_plot_data
{
	std::vector<double> DistrX;
	std::vector<double> DistrY;
	std::vector<double> DistrZ;
};

struct histogram_data
{
	std::vector<double> DistrX;
	std::vector<double> DistrY;
};

class MCMCProjectionCtrl : public WithMCMCProjectionLayout<ParentCtrl>
{
public:
	typedef MCMCProjectionCtrl CLASSNAME;
	
	MCMCProjectionCtrl();
};


class MCMCResultWindow : public WithMCMCResultLayout<TopWindow>
{
public:
	typedef MCMCResultWindow CLASSNAME;
	
	MCMCResultWindow();
	
	MobiView *ParentWindow;
	
	void BeginNewPlots(mcmc_data *Data, double *MinBound, double *MaxBound, const Array<String> &FreeSyms, int RunType);
	void ClearPlots();
	void ResizeChainPlots();
	void RefreshPlots(int CurStep = -1);
	
	void BurninSliderEvent();
	void BurninEditEvent();
	
	void SubBar(Bar &bar);
	
	void SaveResults();
	bool LoadResults();
	
	void MAPToMainPushed();
	void MedianToMainPushed();
	
	
	void GenerateProjectionsPushed();

	//bool HaltWasPushed;
private:
	
	
	void RefreshResultSummary(int CurStep = -1);
	
	ToolBar Tool;
	
	double Burnin[2];
	//double BurninPlotY[2];
	std::vector<double> BurninPlotY;
	
	Array<ScatterCtrl> ChainPlots;
	
	
	mcmc_data *Data = nullptr;
	//TODO: Pack these into Data?
	Array<String> FreeSyms;
	std::vector<double> MinBound;
	std::vector<double> MaxBound;
	
	AutoScroller ChainPlotScroller;
	AutoScroller TrianglePlotScroller;
	
	ParentCtrl ViewChainPlots;
	ParentCtrl ViewTrianglePlots;
	ParentCtrl ViewResultSummary;
	MyRichView ResultSummary;
	Button PushWriteMAP;
	Button PushWriteMedian;
	
	std::vector<double> AcorTimes;
	std::vector<bool>   AcorBelowTolerance;
	
	const int DistrResolution = 20;
	
	//Ugh, why not just use a plot_data_storage for this??
	std::vector<triangle_plot_data> TrianglePlotData;
	Array<TableDataCArray>          TrianglePlotDS;
	Array<ScatterCtrl>              TrianglePlots;
	
	std::vector<histogram_data>     HistogramData;
	Array<ScatterCtrl>              Histograms;
	
	MCMCProjectionCtrl  ViewProjections;
	
	AutoScroller        ProjectionPlotScroll;
	ParentCtrl          ProjectionPlotPane;
	Array<MyPlot>       ProjectionPlots;
	
	AutoScroller        ResidPlotScroll;
	ParentCtrl          ResidPlotPane;
	Array<MyPlot>       ResidPlots;
	Array<MyPlot>       ResidHistograms;
	
	AutoScroller        AutoCorrPlotScroll;
	ParentCtrl          AutoCorrPlotPane;
	Array<MyPlot>       AutoCorrPlots;
	
public:
	std::vector<indexed_parameter> Parameters;
	std::vector<optimization_target> Targets;
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


// TODO: should also have a star option for "favorites".
class Entity_Node : public Upp::Label {
public:
	typedef Entity_Node CLASSNAME;
	
	Entity_Node(Var_Id var_id, const Upp::String &name) : var_id(var_id) { SetText(name); }
	Entity_Node(Entity_Id entity_id, const Upp::String &name) : entity_id(entity_id) { SetText(name); }
	
	union {
		Var_Id var_id;
		Entity_Id entity_id;
	};
};


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

	Upp::ParentCtrl result_selecter_rect;
	Upp::TreeCtrl   result_selecter;
	Upp::Option     show_favorites;
	Upp::TreeCtrl   input_selecter;
	Array<Entity_Node> series_nodes;
	
	Upp::ToolBar    tool_bar;
	
	PlotCtrl        plotter;
	
	EditStatSettings      stat_settings;
	SearchWindow          search_window;
	SensitivityViewWindow sensitivity_window;
	ModelInfoWindow       info_window;
	AdditionalPlotView    additional_plots;
	OptimizationWindow    optimization_window;
	
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
	
private :
	void delete_model();
	bool do_the_load();
};



#endif