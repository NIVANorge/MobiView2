#ifndef _MobiView2_MobiView2_h
#define _MobiView2_MobiView2_h

#include <CtrlLib/CtrlLib.h>


#include <ScatterCtrl/ScatterCtrl.h>
#include <AutoScroller/AutoScroller.h>

#include <map>
#include <vector>


constexpr int MAX_INDEX_SETS = 6;    //NOTE: This has to match the number of (currently) hard coded index set displays in the main window.


#include "StarOption.h"

//#include "Plotting.h"
#include "MyRichView.h"


//#include "PlotCtrl.h"
//#include "ParameterEditing.h"
//#include "MCMC.h"

using MyPlot = Upp::ScatterCtrl; //TODO!

#define LAYOUTFILE <MobiView2/MobiView2.lay>
#include <CtrlCore/lay.h>


//class MobiView2;

#include "ParameterCtrl.h"

/*
class SearchWindow : public WithSearchLayout<TopWindow>
{
public:
	typedef SearchWindow CLASSNAME;
	
	SearchWindow();
	
	MobiView *ParentWindow;
	
	void Find();
	void SelectItem();
};

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

class ModelInfoViewWindow : public WithModelInfoLayout<TopWindow>
{
public:
	typedef ModelInfoViewWindow CLASSNAME;
	
	ModelInfoViewWindow();
	
	MobiView *ParentWindow;
	
	void RefreshText();
};

class EditStatSettingsWindow : public WithEditStatSettingsLayout<TopWindow>
{
public:
	typedef EditStatSettingsWindow CLASSNAME;
	
	EditStatSettingsWindow();
	
	MobiView *ParentWindow;
	
	void LoadData();
	void SaveDataAndClose();
	
	bool ParseDoubleList(String &ListStr, std::vector<double> &Result);
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

#define MAX_ADDITIONAL_PLOTS 10

class MiniPlot : public WithMiniPlotLayout<ParentCtrl>
{
public:
	typedef MiniPlot CLASSNAME;
	
	MiniPlot();
};

class AdditionalPlotView : public WithAdditionalPlotViewLayout<TopWindow>
{
public:
	typedef AdditionalPlotView CLASSNAME;
	
	AdditionalPlotView();
	
	void CopyMainPlot(int Which);
	
	void BuildAll(bool CausedByReRun = false);
	void ClearAll();
	
	MobiView *ParentWindow;
	
	void NumRowsChanged(bool Rebuild = true);
	void UpdateLinkStatus();
	
	void WriteToJson();
	void LoadFromJson();
	
	void SetAll(std::vector<plot_setup> &Setups);
	
private:
	Splitter VerticalSplitter;
	
	MiniPlot Plots[MAX_ADDITIONAL_PLOTS];
	
	ToolBar Tool;
	
	void SubBar(Bar &bar);
};

class StatPlotCtrl : public WithSensitivityStatPlotLayout<ParentCtrl>
{
public:
	typedef StatPlotCtrl CLASSNAME;
	
	StatPlotCtrl();
};

class SensitivityViewWindow : public WithSensitivityLayout<TopWindow>
{
public:
	typedef SensitivityViewWindow CLASSNAME;
	
	SensitivityViewWindow();
	
	MobiView *ParentWindow;
	
	void Update();
	void Run();
	
private :
	Splitter     MainHorizontal;
	MyPlot       Plot;
	StatPlotCtrl StatPlot;
};

class OptimizationParameterSetup : public WithOptimizationLayout<ParentCtrl>
{
public:
	typedef OptimizationParameterSetup CLASSNAME;
	
	OptimizationParameterSetup();
};

class OptimizationTargetSetup : public WithOptimizationTargetLayout<ParentCtrl>
{
public:
	typedef OptimizationTargetSetup CLASSNAME;
	
	OptimizationTargetSetup();
};

class OptimizationRunSetup : public WithOptimizerSetupLayout<ParentCtrl>
{
public:
	typedef OptimizationRunSetup CLASSNAME;
	
	OptimizationRunSetup();
};

class MCMCRunSetup : public WithMCMCSetupLayout<ParentCtrl>
{
public:
	typedef MCMCRunSetup CLASSNAME;
	
	MCMCRunSetup();
};

class SensitivityRunSetup : public WithSensitivitySetupLayout<ParentCtrl>
{
public:
	typedef SensitivityRunSetup CLASSNAME;
	
	SensitivityRunSetup();
};

#include "MCMCErrStruct.h"

enum target_stat_class
{
	StatClass_Unknown,
	StatClass_Stat,
	StatClass_Residual,
	StatClass_LogLikelihood,
};

struct optimization_target
{
	std::string ResultName;
	std::vector<std::string> ResultIndexes;
	std::string InputName;
	std::vector<std::string> InputIndexes;
	std::string ErrParSym;
	std::vector<int> ErrParNum;
	union
	{
		stat_type            Stat;
		residual_type        ResidualStat;
		mcmc_error_structure ErrStruct;
	};
	double Weight;
	std::string Begin;
	std::string End;
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


struct optimization_model;

class OptimizationWindow : public TopWindow
{
public:
	typedef OptimizationWindow CLASSNAME;
	
	OptimizationWindow();
	
	void AddParameterClicked();
	void AddGroupClicked();
	void RemoveParameterClicked();
	void ClearParametersClicked();
	void AddVirtualClicked();
	
	void EnableExpressionsClicked();
	
	void AddTargetClicked();
	void RemoveTargetClicked();
	void ClearTargetsClicked();
	
	void ClearAll();
	
	void RunClicked(int RunMode);
	
	void DisplayClicked();
	
	void SetParameterValues(void *DataSet, double *Pars, size_t NPars, std::vector<indexed_parameter> &Parameters);
	
	void SetError(const String &ErrStr);
	
	MobiView *ParentWindow;
	
	void LoadFromJsonString(String &Json);
	String SaveToJsonString();
	
	void SymbolEdited(int Row);
	void ExprEdited(int Row);
	void ErrSymEdited(int Row);
	void WeightEdited(int Row);
	void StatEdited(int Row);
	void BeginEdited(int Row);
	void EndEdited(int Row);
	
	void SamplerMethodSelected();
	
	bool ErrSymFixup();
	
	//TODO: This should be a general function, not a member of this class...
	void TargetsToPlotSetups(std::vector<optimization_target> &Targets, std::vector<plot_setup> &PlotSetups);
	
private:
	
	bool AddSingleParameter(indexed_parameter &Parameter, int SourceRow, bool ReadAdditionalData=true);
	void AddOptimizationTarget(optimization_target &Target);
	
	void SubBar(Bar &bar);
	void WriteToJson();
	void LoadFromJson();
	
	void TabChange();
	
	bool RunMobiviewMCMC(mcmc_sampler_method Method, double *SamplerParams, size_t NWalkers, size_t NSteps, optimization_model *OptimModel,
		double *InitialValue, double *MinBound, double *MaxBound, int InitialType,
		int CallbackInterval, int RunType);
		
		
	bool RunVarianceBasedSensitivity(int NSamples, int Method, optimization_model *Optim, double *MinBound, double *MaxBound);
	
	
	Array<EditDoubleNotNull> EditMinCtrls;
	Array<EditDoubleNotNull> EditMaxCtrls;
	Array<EditField>         EditSymCtrls;
	Array<EditField>         EditExprCtrls;

	Array<DropList>          TargetStatCtrls;
	Array<EditDoubleNotNull> TargetWeightCtrls;
	Array<EditField>         TargetErrCtrls;
	Array<EditTimeNotNull>   TargetBeginCtrls;
	Array<EditTimeNotNull>   TargetEndCtrls;
	
	Array<EditDoubleNotNull> MCMCSamplerParamCtrls;
	
	ToolBar Tool;
	
	Splitter MainVertical;
	OptimizationParameterSetup ParSetup;
	OptimizationTargetSetup    TargetSetup;
	
public:
	OptimizationRunSetup       RunSetup;
	MCMCRunSetup               MCMCSetup;
	SensitivityRunSetup        SensitivitySetup;
	
	mcmc_data                  Data;
	
	std::vector<indexed_parameter> Parameters;
	std::vector<optimization_target> Targets;
};

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

struct Mobius_Model;
struct Model_Application;
struct Data_Set;

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
	
	Upp::ToolBar    tool_bar;
	
	//PlotCtrl Plotter;
	Upp::ScatterCtrl plotter;  //TODO: replace with PlotCtrl again!
	
	
	Mobius_Model      *model = nullptr;
	Data_Set          *data_set = nullptr;
	Model_Application *app = nullptr;
	
	
	
	
	void sub_bar(Upp::Bar &bar);
	
	void log(Upp::String msg, bool error = false);
	void log_warnings_and_errors();
	
	void load();
	void save_parameters();
	void save_parameters_as();
	void run_model();
	/*
	void SaveBaseline();
	void RevertBaseline();
	bool BaselineWasJustSaved = false;
	
	void SaveInputsAsDat();
	
	void StoreSettings(bool OverwriteFavorites = true);
	*/
	void clean_interface();
	void build_interface();
	void closing_checks();
	
	void store_settings(bool store_favorites = true);
	
	void plot_rebuild();
	
	/*
	StatisticsSettings StatSettings;
	void OpenStatSettings();
	EditStatSettingsWindow EditStatSettings;
	
	void OpenSearch();
	SearchWindow Search;
	
	void OpenStructureView();
	StructureViewWindow StructureView;
	
	void OpenChangeIndexes();
	ChangeIndexesWindow ChangeIndexes;
	
	void OpenAdditionalPlotView();
	AdditionalPlotView OtherPlots;
	
	void OpenModelInfoView();
	ModelInfoViewWindow ModelInfo;
	
	void OpenSensitivityView();
	SensitivityViewWindow SensitivityWindow;
	
	void OpenOptimizationView();
	OptimizationWindow OptimizationWin;
	
	MCMCResultWindow MCMCResultWin;
	
	VarianceSensitivityWindow VarSensitivityWin;
	
	void AddParameterGroupsRecursive(int ParentId, const char *ParentName, int ChildCount);
	*/
	
	
	/*
	void PlotModeChange();
	void PlotRebuild();
	void UpdateEquationSelecter();
	
	bool GetIndexSetsForSeries(void *DataSet, std::string &Name, int Type, std::vector<char *> &IndexSetsOut);
	bool GetSelectedIndexesForSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, int Type, std::vector<char *> &IndexesOut);
	
	void GetSingleSelectedResultSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, String &Legend, String &Unit, double *WriteTo);
	void GetSingleSelectedInputSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, String &Legend, String &Unit, double *WriteTo, bool AlignWithResults);
	
	void GetSingleResultSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, double *WriteTo, size_t SelectRowFor, std::string &Row);
	void GetSingleInputSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, double *WriteTo, size_t SelectRowFor, std::string &Row);

	
	void ExpandIndexSetClicked(size_t IndexSet);
	void RefreshParameterView(bool RefreshValuesOnly = false);
	
	
	bool GetSelectedParameterGroupIndexSets(std::vector<char *> &IndexSetsOut, String &GroupNameOut);
	int FindSelectedParameterRow();
	indexed_parameter GetParameterAtRow(int Row);
	indexed_parameter GetSelectedParameter();
	
	indexed_parameter CurrentSelectedParameter = {};
	
	int ExpandedSetLocal;
	int SecondExpandedSetLocal;
	//void RecursiveUpdateParameter(int Level, std::vector<std::string> &CurrentIndexes, const indexed_parameter &Parameter, void *DataSet, Value Val);
	void ParameterEditAccepted(const indexed_parameter &Parameter, void *DataSet, Value Val, bool UpdateLock=false);
	
	
	void GetResultDataRecursive(std::string &Name, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps, std::vector<std::vector<double>> &PushTo, std::vector<std::string> &PushNamesTo);
	void SaveToCsv();
	
	void GetGofOffsets(const Time &ReferenceTime, uint64 ReferenceTimesteps, Time &BeginOut, Time &EndOut, int64 &GofOffsetOut, int64 &GofTimestepsOut);
	void GetGofOffsetsBase(const Time &AttemptBegin, const Time &AttemptEnd, const Time &ReferenceTime, uint64 ReferenceTimesteps, Time &BeginOut, Time &EndOut, int64 &GofOffsetOut, int64 &GofTimestepsOut);
	*/

	
	
	// TODO: These should probably be owned by ParameterCtrl instead??
	//Array<Ctrl> ParameterControls;
	//std::vector<parameter_type> CurrentParameterTypes;
	
	//Array<Ctrl> EquationSelecterFavControls;
	
	//model_dll_interface ModelDll;
	//timestep_size TimestepSize;
	
	Upp::Label     *index_set_name[MAX_INDEX_SETS]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	Upp::DropList  *index_list[MAX_INDEX_SETS];
	Upp::Option    *index_lock[MAX_INDEX_SETS];
	Upp::Option    *index_expand[MAX_INDEX_SETS];
	
	std::map<std::string, size_t> index_set_name_to_pos;
	
	
	//void *DataSet = nullptr;
	//void *BaselineDataSet = nullptr;
	
	
	//bool ParametersWereChangedSinceLastSave = false;
	
	
	//NOTE: These should hold the names of what files were last opened (in an earlier
	//session or this session), regardless of whether or not files are currently open.)
	//std::string DllFile;
	//std::string InputFile;
	//std::string ParameterFile;
	std::string model_file;
	std::string data_file;
};



#endif