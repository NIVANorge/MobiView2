#include "OptimizationWindow.h"
#include "MobiView2.h"

/*
#ifdef _WIN32
	#include <winsock2.h>          //NOTE: For some reason dlib includes some windows headers in an order that upp's clang setup doesn't like
#endif

#define DLIB_NO_GUI_SUPPORT  //NOTE: Turns off dlib's own GUI since we are using upp.

#include "dlib/optimization.h"
#include "dlib/global_optimization.h"

#include <unordered_map>
*/

#define IMAGECLASS IconImg4
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

using namespace Upp;

OptimizationParameterSetup::OptimizationParameterSetup() {
	CtrlLayout(*this);
}

OptimizationTargetSetup::OptimizationTargetSetup() {
	CtrlLayout(*this);
}

OptimizationRunSetup::OptimizationRunSetup() {
	CtrlLayout(*this);
}

MCMCRunSetup::MCMCRunSetup() {
	CtrlLayout(*this);
}

SensitivityRunSetup::SensitivityRunSetup() {
	CtrlLayout(*this);
}

OptimizationWindow::OptimizationWindow(MobiView2 *parent) : parent(parent) {
	SetRect(0, 0, 740, 740);
	Title("MobiView optimization and MCMC setup").Sizeable().Zoomable();
	
	par_setup.parameter_view.AddColumn(Id("__name"), "Name");
	par_setup.parameter_view.AddColumn(Id("__indexes"), "Indexes");
	par_setup.parameter_view.AddColumn(Id("__min"), "Min");
	par_setup.parameter_view.AddColumn(Id("__max"), "Max");
	par_setup.parameter_view.AddColumn(Id("__unit"), "Unit");
	par_setup.parameter_view.AddColumn(Id("__sym"), "Symbol");
	par_setup.parameter_view.AddColumn(Id("__expr"), "Expression");
	
	par_setup.option_use_expr.Set((int)false);
	par_setup.option_use_expr.WhenAction     = THISBACK(enable_expressions_clicked);
	par_setup.parameter_view.HeaderObject().HideTab(5);
	par_setup.parameter_view.HeaderObject().HideTab(6);
	
	target_setup.target_view.AddColumn(Id("__resultname"), "Result name");
	target_setup.target_view.AddColumn(Id("__resultindexes"), "Result idxs.");
	target_setup.target_view.AddColumn(Id("__inputname"), "Input name");
	target_setup.target_view.AddColumn(Id("__inputindexes"), "Input idxs.");
	target_setup.target_view.AddColumn(Id("__targetstat"), "Statistic");
	target_setup.target_view.AddColumn(Id("__errparam"), "Error param(s).");
	target_setup.target_view.AddColumn(Id("__weight"), "Weight");
	target_setup.target_view.AddColumn(Id("__begin"), "Begin");
	target_setup.target_view.AddColumn(Id("__end"), "End");
	
	target_setup.edit_timeout.SetData(-1);
	
	par_setup.push_add_parameter.WhenPush    = THISBACK(add_parameter_clicked);
	par_setup.push_add_group.WhenPush        = THISBACK(add_group_clicked);
	par_setup.push_remove_parameter.WhenPush = THISBACK(remove_parameter_clicked);
	par_setup.push_clear_parameters.WhenPush = THISBACK(clear_parameters_clicked);
	par_setup.push_add_virtual.WhenPush      = THISBACK(add_virtual_clicked);
	
	par_setup.push_add_parameter.SetImage(IconImg4::Add());
	par_setup.push_add_group.SetImage(IconImg4::AddGroup());
	par_setup.push_remove_parameter.SetImage(IconImg4::Remove());
	par_setup.push_clear_parameters.SetImage(IconImg4::RemoveGroup());
	par_setup.push_add_virtual.SetImage(IconImg4::Add());
	par_setup.push_add_virtual.Disable();
	par_setup.push_add_virtual.Hide();
	
	target_setup.push_add_target.WhenPush    = THISBACK(add_target_clicked);
	target_setup.push_remove_target.WhenPush = THISBACK(remove_target_clicked);
	target_setup.push_clear_targets.WhenPush = THISBACK(clear_targets_clicked);
	target_setup.push_display.WhenPush       = THISBACK(display_clicked);
	
	target_setup.push_add_target.SetImage(IconImg4::Add());
	target_setup.push_remove_target.SetImage(IconImg4::Remove());
	target_setup.push_clear_targets.SetImage(IconImg4::RemoveGroup());
	target_setup.push_display.SetImage(IconImg4::ViewMorePlots());
	
	run_setup.push_run.WhenPush          = [&](){ run_clicked(0); };
	run_setup.push_run.SetImage(IconImg4::Run());
	
	run_setup.edit_max_evals.Min(1);
	run_setup.edit_max_evals.SetData(1000);
	run_setup.edit_epsilon.Min(0.0);
	run_setup.edit_epsilon.SetData(0.0);
	
	
	mcmc_setup.push_run.WhenPush         = [&](){ run_clicked(1); };
	mcmc_setup.push_run.SetImage(IconImg4::Run());
	//mcmc_setup.push_view_chains.WhenPush << [&]() { if(!ParentWindow->MCMCResultWin.IsOpen()) ParentWindow->MCMCResultWin.Open(); }; //TODO
	mcmc_setup.push_view_chains.SetImage(IconImg4::ViewMorePlots());
	mcmc_setup.push_extend_run.WhenPush   = [&](){ run_clicked(2); };
	mcmc_setup.push_extend_run.Disable();
	mcmc_setup.push_extend_run.SetImage(IconImg4::Run());
	
	mcmc_setup.edit_steps.Min(10);
	mcmc_setup.edit_steps.SetData(1000);
	mcmc_setup.edit_walkers.Min(10);
	mcmc_setup.edit_walkers.SetData(25);
	mcmc_setup.initial_type_switch.SetData(0);
	
	mcmc_setup.sampler_param_view.AddColumn("Name");
	mcmc_setup.sampler_param_view.AddColumn("Value");
	mcmc_setup.sampler_param_view.AddColumn("Description");
	mcmc_setup.sampler_param_view.ColumnWidths("3 2 5");
	
	//TODO!
	/*
	mcmc_setup.select_sampler.Add((int)MCMCMethod_AffineStretch,         "Affine stretch (recommended)");
	mcmc_setup.select_sampler.Add((int)MCMCMethod_AffineWalk,            "Affine walk");
	mcmc_setup.select_sampler.Add((int)MCMCMethod_DifferentialEvolution, "Differential evolution");
	mcmc_setup.select_sampler.Add((int)MCMCMethod_MetropolisHastings,    "Metropolis-Hastings (independent chains)");
	mcmc_setup.select_sampler.GoBegin();
	mcmc_setup.select_sampler.WhenAction << THISBACK(SamplerMethodSelected);
	sampler_method_selected(); //To set the sampler parameters for the initial selection
	*/
	
	sensitivity_setup.edit_sample_size.Min(10);
	sensitivity_setup.edit_sample_size.SetData(1000);
	sensitivity_setup.push_run.WhenPush  = [&](){ run_clicked(3); };
	//TODO
	//sensitivity_setup.push_view_results.WhenPush = [&]() {	if(!ParentWindow->VarSensitivityWin.IsOpen()) ParentWindow->VarSensitivityWin.Open(); };
	sensitivity_setup.push_run.SetImage(IconImg4::Run());
	sensitivity_setup.push_view_results.SetImage(IconImg4::ViewMorePlots());
	
	sensitivity_setup.select_method.Add(0, "Latin hypercube");
	sensitivity_setup.select_method.Add(1, "Independent uniform");
	sensitivity_setup.select_method.GoBegin();
	
	AddFrame(tool);
	tool.Set(THISBACK(sub_bar));
	
	target_setup.optimizer_type_tab.Add(run_setup.SizePos(), "Optimizer");
	target_setup.optimizer_type_tab.Add(mcmc_setup.SizePos(), "MCMC");
	target_setup.optimizer_type_tab.Add(sensitivity_setup.SizePos(), "Variance based sensitivity");
	target_setup.optimizer_type_tab.WhenSet << THISBACK(tab_change);
	
	main_vertical.Vert();
	main_vertical.Add(par_setup);
	main_vertical.Add(target_setup);
	Add(main_vertical.SizePos());
}

void OptimizationWindow::set_error(const String &err_str) {
	target_setup.error_label.SetText(err_str);
}

void OptimizationWindow::sub_bar(Bar &bar) {
	bar.Add(IconImg4::Open(), THISBACK(read_from_json)).Tip("Load setup from file");
	bar.Add(IconImg4::Save(), THISBACK(write_to_json)).Tip("Save setup to file");
}

void OptimizationWindow::enable_expressions_clicked() {
	bool val = (bool)par_setup.option_use_expr.Get();
	
	if(val) {
		par_setup.parameter_view.HeaderObject().ShowTab(5);
		par_setup.parameter_view.HeaderObject().ShowTab(6);
		par_setup.push_add_virtual.Enable();
		par_setup.push_add_virtual.Show();
	} else {
		par_setup.parameter_view.HeaderObject().HideTab(5);
		par_setup.parameter_view.HeaderObject().HideTab(6);
		par_setup.push_add_virtual.Disable();
		par_setup.push_add_virtual.Hide();
	}
}

bool OptimizationWindow::add_single_parameter(Indexed_Parameter parameter) {
	if(!is_valid(parameter.id) && !parameter.virt) return false; //TODO: Some kind of error message explaining how to use the feature?
	
	auto app = parent->app;
	
	Entity_Registration<Reg_Type::parameter> *par;
	if(!parameter.virt) {
		par = app->model->find_entity<Reg_Type::parameter>(parameter.id);
		if(par->decl_type != Decl_Type::par_real) return false; //TODO: Dlib has provision for allowing integer parameters
	}
	
	int overwrite_row = -1;
	int row = 0;
	if(!parameter.virt) {
		for(Indexed_Parameter &other_par : parameters) {
			if(parameter_is_subset_of(parameter, other_par)) return false;
			if(parameter_is_subset_of(other_par, parameter)) {
				other_par = parameter;
				overwrite_row = row;
			}
			++row;
		}
	}
	
	String index_str = make_parameter_index_string(&app->parameter_structure, &parameter);
	if(overwrite_row < 0) {
		double min = Null;
		double max = Null;
		String name = "(virtual)";
		String unit = "";
		String sym  = parameter.symbol.data();
		String expr = parameter.expr.data();
		
		if(!parameter.virt) {
			min     = par->min_val.val_real;
			max     = par->max_val.val_real;
			//unit = ; //TODO
			sym     = str(par->handle_name);
			name    = str(par->name);
		}
		
		parameter.symbol = sym.ToStd();
		parameters.push_back(std::move(parameter));
		int row = parameters.size()-1;
		
		par_setup.parameter_view.Add(name, index_str, min, max, unit, sym, expr);

		edit_min_ctrls.Create<EditDoubleNotNull>();
		edit_max_ctrls.Create<EditDoubleNotNull>();
		par_setup.parameter_view.SetCtrl(row, 2, edit_min_ctrls[row]);
		par_setup.parameter_view.SetCtrl(row, 3, edit_max_ctrls[row]);
		
		edit_sym_ctrls.Create<EditField>();
		edit_expr_ctrls.Create<EditField>();
		par_setup.parameter_view.SetCtrl(row, 5, edit_sym_ctrls[row]);
		par_setup.parameter_view.SetCtrl(row, 6, edit_expr_ctrls[row]);
		
		edit_sym_ctrls[row].WhenAction <<  [this, row](){ symbol_edited(row); };
		edit_expr_ctrls[row].WhenAction << [this, row](){ expr_edited(row); };
	}
	else
		par_setup.parameter_view.Set(overwrite_row, 1, index_str);
	
	return true;
}

void OptimizationWindow::symbol_edited(int row) {
	parameters[row].symbol = par_setup.parameter_view.Get(row, "__sym").ToStd();
}

void OptimizationWindow::expr_edited(int row) {
	parameters[row].expr = par_setup.parameter_view.Get(row, "__expr").ToStd();
}

void OptimizationWindow::add_parameter_clicked() {
	Indexed_Parameter &par = parent->params.get_selected_parameter();
	add_single_parameter(par);
}

void OptimizationWindow::add_virtual_clicked() {
	Indexed_Parameter par = {};
	par.virt = true;
	add_single_parameter(par);
}

void OptimizationWindow::add_group_clicked() {
	for(Indexed_Parameter &par : parent->params.listed_pars)
		add_single_parameter(par);
}

void OptimizationWindow::remove_parameter_clicked() {
	int sel_row = par_setup.parameter_view.GetCursor();
	/*
	for(int Row = 0; Row < ParSetup.ParameterView.GetCount(); ++Row)
	{
		if(ParSetup.ParameterView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	*/
	if(sel_row < 0) return;
	
	par_setup.parameter_view.Remove(sel_row);
	parameters.erase(parameters.begin()+sel_row);
	edit_min_ctrls.Remove(sel_row);
	edit_max_ctrls.Remove(sel_row);
	edit_sym_ctrls.Remove(sel_row);
	edit_expr_ctrls.Remove(sel_row);
}

void OptimizationWindow::clear_parameters_clicked() {
	par_setup.parameter_view.Clear();
	parameters.clear();
	edit_min_ctrls.Clear();
	edit_max_ctrls.Clear();
	edit_sym_ctrls.Clear();
	edit_expr_ctrls.Clear();
}


void OptimizationWindow::add_optimization_target(Optimization_Target &target) {
	targets.push_back(target);
	
	//TODO!
	/*
	String InputIndexStr = MakeIndexString(Target.InputIndexes);
	String ResultIndexStr = MakeIndexString(Target.ResultIndexes);
	
	int TargetStat = (int)Target.Stat;;
	
	TargetSetup.TargetView.Add(Target.ResultName.data(), ResultIndexStr, Target.InputName.data(), InputIndexStr, TargetStat, Target.ErrParSym.data(), Target.Weight, Target.Begin.data(), Target.End.data());
	
	TargetStatCtrls.Create<DropList>();
	DropList &SelectStat = TargetStatCtrls.Top();
	
	TargetErrCtrls.Create<EditField>();
	EditField &ErrCtrl = TargetErrCtrls.Top();
	
	int TabNum = TargetSetup.OptimizerTypeTab.Get();
	
	if(TabNum == 2)
	{
		#define SET_SETTING(Handle, Name, Type) SelectStat.Add((int)StatType_##Handle, Name);
		#define SET_RES_SETTING(Handle, Name, Type)
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
	}
	if(TabNum == 0 || TabNum == 2)
	{
		#define SET_SETTING(Handle, Name, Type)
		#define SET_RES_SETTING(Handle, Name, Type) if(Type != -1) SelectStat.Add((int)ResidualType_##Handle, Name);
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
	}
	if(TabNum == 0 || TabNum == 1)
	{
		#define SET_LL_SETTING(Handle, Name, NumErr) SelectStat.Add((int)MCMCError_##Handle, Name);
		#include "LLSettings.h"
		#undef SET_LL_SETTING
	}
	
	
	int Row = TargetSetup.TargetView.GetCount() - 1;
	int Col = TargetSetup.TargetView.GetPos(Id("__targetstat"));
	TargetSetup.TargetView.SetCtrl(Row, Col, SelectStat);
	SelectStat.WhenAction << [this, Row](){ StatEdited(Row); };
	
	Col     = TargetSetup.TargetView.GetPos(Id("__errparam"));
	TargetSetup.TargetView.SetCtrl(Row, Col, ErrCtrl);
	ErrCtrl.WhenAction << [this, Row](){ ErrSymEdited(Row); };
	
	TargetWeightCtrls.Create<EditDoubleNotNull>();
	EditDoubleNotNull &EditWt = TargetWeightCtrls.Top();
	EditWt.Min(0.0);
	Col     = TargetSetup.TargetView.GetPos(Id("__weight"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditWt);
	EditWt.WhenAction << [this, Row](){ WeightEdited(Row); };
	
	TargetBeginCtrls.Create<EditTimeNotNull>();
	EditTimeNotNull &EditBegin = TargetBeginCtrls.Top();
	Col = TargetSetup.TargetView.GetPos(Id("__begin"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditBegin);
	EditBegin.WhenAction << [this, Row](){ BeginEdited(Row); };
	
	TargetEndCtrls.Create<EditTimeNotNull>();
	EditTimeNotNull &EditEnd = TargetEndCtrls.Top();
	Col = TargetSetup.TargetView.GetPos(Id("__end"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditEnd);
	EditEnd.WhenAction << [this, Row](){ EndEdited(Row); };
	*/
}

void OptimizationWindow::stat_edited(int row) {
	targets[row].stat_type = (int)target_setup.target_view.Get(row, "__targetstat");
}

void OptimizationWindow::err_sym_edited(int row) {
	//TODO
	//Targets[Row].ErrParSym = TargetSetup.TargetView.Get(Row, "__errparam").ToStd();
}

void OptimizationWindow::weight_edited(int row) {
	targets[row].weight = (double)target_setup.target_view.Get(row, "__weight");
}

void OptimizationWindow::begin_edited(int row) {
	targets[row].start = convert_time((Time)target_setup.target_view.Get(row, "__begin"));
}

void OptimizationWindow::end_edited(int row) {
	targets[row].end   = convert_time((Time)target_setup.target_view.Get(row, "__end"));
}

void OptimizationWindow::add_target_clicked() {
	//TODO
	/*
	plot_setup PlotSetup;
	ParentWindow->Plotter.GatherCurrentPlotSetup(PlotSetup);
	
	if(PlotSetup.SelectedResults.size() != 1 || PlotSetup.SelectedInputs.size() > 1)
	{
		SetError("This only works with a single selected result series and at most one input series.");
		return;
	}
	
	for(int Idx = 0; Idx < PlotSetup.SelectedIndexes.size(); ++Idx)
	{
		if(PlotSetup.SelectedIndexes[Idx].size() != 1 && PlotSetup.IndexSetIsActive[Idx])
		{
			SetError("This currently only works with a single selected index combination");
			return;
		}
	}
	
	optimization_target Target = {};
	Target.ResultName = PlotSetup.SelectedResults[0];
	std::vector<char *> ResultIndexes;
	bool Success      = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, Target.ResultName, 0, ResultIndexes);
	if(!Success) return;
	//Ugh, super annoying to have to convert back and forth between char* and string to ensure
	//storage...
	for(const char *Idx : ResultIndexes)
			Target.ResultIndexes.push_back(std::string(Idx));
	
	if(PlotSetup.SelectedInputs.size() == 1)
	{
		Target.InputName  = PlotSetup.SelectedInputs[0];
		std::vector<char *> InputIndexes;
		bool Success = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, Target.InputName, 1, InputIndexes);
		if(!Success) return;
		for(const char *Idx : InputIndexes)
			Target.InputIndexes.push_back(std::string(Idx));
	}
	
	int TabNum = TargetSetup.OptimizerTypeTab.Get(); 
	//NOTE: Defaults.
	if(TabNum == 0) Target.ResidualStat = ResidualType_MAE;
	else if(TabNum == 1) Target.ErrStruct = MCMCError_NormalHet1; //This is the easiest one to start with maybe. Or should we just do Normal?
	else if(TabNum == 2) Target.Stat = StatType_Mean;

	Target.Weight = 1.0;
	
	//NOTE for you to be able to add a target, the model has to have been run once any way, so
	//there is no problem using these time stamps for that.
	char TimeStr[256];
	uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(ParentWindow->DataSet);
	ParentWindow->ModelDll.GetStartDate(ParentWindow->DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);

	Time GofStartTime, GofEndTime;
	int64 GofOffset, GofTimesteps;
	ParentWindow->GetGofOffsets(ResultStartTime, ResultTimesteps, GofStartTime, GofEndTime, GofOffset, GofTimesteps);
	Target.Begin = Format(GofStartTime).ToStd();
	Target.End   = Format(GofEndTime).ToStd();
	
	AddOptimizationTarget(Target);
	*/
}
/*
void OptimizationWindow::TargetsToPlotSetups(std::vector<optimization_target> &Targets, std::vector<plot_setup> &PlotSetups)
{
	PlotSetups.clear();
	PlotSetups.reserve(Targets.size());
	
	for(optimization_target &Target : Targets)
	{
		plot_setup PlotSetup = {};
		PlotSetup.SelectedIndexes.resize(MAX_INDEX_SETS);
		PlotSetup.IndexSetIsActive.resize(MAX_INDEX_SETS);
		
		PlotSetup.MajorMode = MajorMode_Regular;//MajorMode_Residuals;
		PlotSetup.AggregationPeriod = Aggregation_None;
		PlotSetup.ScatterInputs = true;
		
		std::vector<char *> IndexSets;
		
		if(Target.InputName != "")
		{
			PlotSetup.SelectedInputs.push_back(Target.InputName);
		
			bool Success = ParentWindow->GetIndexSetsForSeries(ParentWindow->DataSet, Target.InputName, 1, IndexSets);
			if(!Success) return;
			
			for(int IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
			{
				size_t Id = ParentWindow->IndexSetNameToId[IndexSets[IdxIdx]];
				PlotSetup.IndexSetIsActive[Id] = true;
				PlotSetup.SelectedIndexes[Id].push_back(Target.InputIndexes[IdxIdx]);
			}
		}

		PlotSetup.SelectedResults.push_back(Target.ResultName);

		IndexSets.clear();
		bool Success = ParentWindow->GetIndexSetsForSeries(ParentWindow->DataSet, Target.ResultName, 0, IndexSets);
		if(!Success) return;
		
		for(int IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
		{
			size_t Id = ParentWindow->IndexSetNameToId[IndexSets[IdxIdx]];
			PlotSetup.IndexSetIsActive[Id] = true;
			PlotSetup.SelectedIndexes[Id].push_back(Target.ResultIndexes[IdxIdx]);
		}
		
		PlotSetups.push_back(PlotSetup);
	}
}
*/

void OptimizationWindow::display_clicked() {
	
	if(targets.empty()) {
		set_error("There are no targets to display the plots of");
		return;
	}
	
	set_error("");
	
	std::vector<Plot_Setup> plot_setups;
	//TargetsToPlotSetups(Targets, PlotSetups);   //TODO!

	parent->additional_plots.set_all(plot_setups);
	parent->open_additional_plots();
}

void OptimizationWindow::remove_target_clicked() {
	int sel_row = target_setup.target_view.GetCursor();
	/*
	for(int Row = 0; Row < TargetSetup.TargetView.GetCount(); ++Row)
	{
		if(TargetSetup.TargetView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	*/
	if(sel_row < 0) return;
	
	target_setup.target_view.Remove(sel_row);
	targets.erase(targets.begin()+sel_row);
	target_weight_ctrls.Remove(sel_row);
	target_stat_ctrls.Remove(sel_row);
	target_err_ctrls.Remove(sel_row);
	target_begin_ctrls.Remove(sel_row);
	target_end_ctrls.Remove(sel_row);
}

void OptimizationWindow::clear_targets_clicked() {
	target_setup.target_view.Clear();
	targets.clear();
	target_weight_ctrls.Clear();
	target_stat_ctrls.Clear();
	target_err_ctrls.Clear();
	target_begin_ctrls.Clear();
	target_end_ctrls.Clear();
}
	
void OptimizationWindow::clean() {
	clear_parameters_clicked();
	clear_targets_clicked();
}

void OptimizationWindow::sampler_method_selected() {
	//TODO
	/*
	mcmc_sampler_method Method = (mcmc_sampler_method)(int)MCMCSetup.SelectSampler.GetData();
	
	MCMCSetup.SamplerParamView.Clear();
	MCMCSamplerParamCtrls.Clear();
	
	switch(Method)
	{
		case MCMCMethod_AffineStretch :
		{
			MCMCSetup.SamplerParamView.Add("a", 2.0, "Max. stretch factor");
		} break;
		
		case MCMCMethod_AffineWalk :
		{
			MCMCSetup.SamplerParamView.Add("|S|", 10.0, "Size of sub-ensemble. Has to be between 2 and NParams/2.");
		} break;
		
		case MCMCMethod_DifferentialEvolution :
		{
			MCMCSetup.SamplerParamView.Add("\u03B3", -1.0, "Stretch factor. If negative, will be set to default of 2.38/sqrt(2*dim)");
			MCMCSetup.SamplerParamView.Add("b", 1e-3, "Max. random walk step (multiplied by |max - min| for each par.)");
			MCMCSetup.SamplerParamView.Add("CR", 1.0, "Crossover probability");
		} break;
		
		case MCMCMethod_MetropolisHastings :
		{
			MCMCSetup.SamplerParamView.Add("b", 0.02, "Std. dev. of random walk step (multiplied by |max - min| for each par.)");
		} break;
	}
	
	int Rows = MCMCSetup.SamplerParamView.GetCount();
	MCMCSamplerParamCtrls.InsertN(0, Rows);
	for(int Row = 0; Row < Rows; ++Row)
		MCMCSetup.SamplerParamView.SetCtrl(Row, 1, MCMCSamplerParamCtrls[Row]);
	*/
}

/*
struct mcmc_run_data
{
	optimization_model *Model;
	mcmc_data          *Data;
	std::vector<void *> DataSets;
	double *MinBound;
	double *MaxBound;
};

struct mcmc_callback_data
{
	MobiView *ParentWindow;
};

bool MCMCCallbackFun(void *CallbackData, int CurStep)
{
	mcmc_callback_data *CBD = (mcmc_callback_data *)CallbackData;
	
	
	//if(CBD->ParentWindow->MCMCResultWin.HaltWasPushed)
	//	return false;
	
	GuiLock Lock;
	
	CBD->ParentWindow->MCMCResultWin.RefreshPlots(CurStep);
	CBD->ParentWindow->ProcessEvents();
	
	return true;
}

double MCMCLogLikelyhoodEval(void *RunData, int Walker, int Step)
{
	mcmc_run_data *RunData0 = (mcmc_run_data *)RunData;
	mcmc_data     *Data     = RunData0->Data;
	
	column_vector Pars(Data->NPars);
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		double Val = (*Data)(Walker, Par, Step);
		
		if(Val < RunData0->MinBound[Par] || Val > RunData0->MaxBound[Par])
			return -std::numeric_limits<double>::infinity();
		
		Pars(Par) = Val;
	}
	
	return RunData0->Model->EvaluateObjectives(RunData0->DataSets[Walker], Pars);
}

bool OptimizationWindow::RunMobiviewMCMC(mcmc_sampler_method Method, double *SamplerParams, size_t NWalkers0, size_t NSteps, optimization_model *OptimModel,
	double *InitialValue, double *MinBound, double *MaxBound, int InitialType, int CallbackInterval, int RunType)
{
	size_t NPars = OptimModel->FreeParCount;
	size_t NWalkers = NWalkers0;
	
	int InitialStep = 0;
	{
		GuiLock Lock;
		
		MCMCResultWindow *ResultWin = &ParentWindow->MCMCResultWin;
		ResultWin->ClearPlots();
		
		if(RunType == 2) // NOTE RunType==2 means extend the previous run.
		{
			if(Data.NSteps == 0)
			{
				SetError("Can't extend a run before the model has been run once");
				return false;
			}
			if(Data.NSteps >= NSteps)
			{
				SetError(Format("To extend the run, you must set a higher number of timesteps than previously. Previous was %d.", (int)Data.NSteps));
				return false;
			}
			if(Data.NWalkers != NWalkers)
			{
				SetError("To extend the run, you must have the same amount of walkers as before.");
				return false;
			}
			if(ResultWin->Parameters != Parameters || ResultWin->Targets != Targets)
			{
				//TODO: Could alternatively let you continue with the old parameter set.
				SetError("You can't extend the run since the parameters or targets have changed.");
				return false;
			}
			InitialStep = Data.NSteps-1;
			
			Data.ExtendSteps(NSteps);
		}
		else
		{
			Data.Free();
			Data.Allocate(NWalkers, NPars, NSteps);
		}
		
		Array<String> FreeSyms;
		for(indexed_parameter &Parameter : Parameters)
			if(Parameter.Expr == "") FreeSyms.push_back(String(Parameter.Symbol.data()));
		
		
		//NOTE: These have to be cached so that we don't have to rely on the optimization window
		//not being edited (and going out of sync with the data)
		ResultWin->Parameters = Parameters;
		ResultWin->Targets    = Targets;
		
		ResultWin->ChoosePlotsTab.Set(0);
		ResultWin->BeginNewPlots(&Data, MinBound, MaxBound, FreeSyms, RunType);
		
		ParentWindow->ProcessEvents();
		
		if(RunType==1)
		{
			std::mt19937_64 Generator;
			for(int Walker = 0; Walker < NWalkers; ++Walker)
			{
				for(int Par = 0; Par < NPars; ++Par)
				{
					double Initial = InitialValue[Par];
					double Draw = Initial;
					if(InitialType == 0)
					{
						//Initializing walkers to a gaussian ball around the initial parameter values.
						double StdDev = (MaxBound[Par] - MinBound[Par])/400.0;   //TODO: How to choose scaling coefficient?
						
						std::normal_distribution<double> Distribution(Initial, StdDev);
						
						Draw = Distribution(Generator);
						Draw = std::max(MinBound[Par], Draw);
						Draw = std::min(MaxBound[Par], Draw);
			
					}
					else if(InitialType == 1)
					{
						std::uniform_real_distribution<double> Distribution(MinBound[Par], MaxBound[Par]);
						Draw = Distribution(Generator);
					}
					else
						PromptOK("Internal error, wrong intial type");
					
					Data(Walker, Par, 0) = Draw;
				}
			}
		}
	}
	
	ParentWindow->ProcessEvents();
	
	std::vector<double> Scales(NPars);
	for(int Par = 0; Par < NPars; ++Par)
	{
		Scales[Par] = MaxBound[Par] - MinBound[Par];
		assert(Scales[Par] >= 0.0);
	}
	
	mcmc_run_data RunData;
	RunData.Model = OptimModel;
	RunData.Data  = &Data;
	RunData.MinBound = MinBound;
	RunData.MaxBound = MaxBound;
	RunData.DataSets.resize(NWalkers);
	
	//TODO: For Emcee we really only need a set of datasets that is the size of a
	//partial ensemble, which is about half of the total ensemble.
	for(int Walker = 0; Walker < NWalkers; ++Walker)
		RunData.DataSets[Walker] = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false, true);
	
	mcmc_callback_data CallbackData;
	CallbackData.ParentWindow = ParentWindow;
	
	//TODO: We have to check the SamplerParams for correctness somehow!
	
	bool Finished = RunMCMC(Method, SamplerParams, Scales.data(), MCMCLogLikelyhoodEval, (void *)&RunData, &Data, MCMCCallbackFun, (void *)&CallbackData, CallbackInterval, InitialStep);
	
	for(int Walker = 0; Walker < NWalkers; ++Walker)
		ParentWindow->ModelDll.DeleteDataSet(RunData.DataSets[Walker]);
	
	return Finished;
}

bool OptimizationWindow::RunVarianceBasedSensitivity(int NSamples, int Method, optimization_model *Optim, double *MinBound, double *MaxBound)
{
	
	int ProgressInterval = 50; //TODO: Make the caller set this.
	
	VarianceSensitivityWindow &SensWin = ParentWindow->VarSensitivityWin;
	if(!SensWin.IsOpen()) SensWin.Open();
	
	SensWin.Plot.ClearAll(true);
	SensWin.Plot.SetLabels("Combined statistic", "Frequency");
	
	SensWin.ShowProgress.Show();
	SensWin.ResultData.Clear();
	
	for(indexed_parameter &Par : *Optim->Parameters)
	{
		if(Par.Expr == "") SensWin.ResultData.Add(Par.Symbol.data(), Null, Null);
	}
	ParentWindow->ProcessEvents();
	
	int NPars = Optim->FreeParCount;
	
	//NOTE: We re-appropriate the MCMC data for use here.
	mcmc_data MatA;
	mcmc_data MatB;
	MatA.Allocate(1, NPars, NSamples);
	MatB.Allocate(1, NPars, NSamples);
	
	if(Method == 0)
	{
		DrawLatinHyperCubeSamples(&MatA, MinBound, MaxBound);
		DrawLatinHyperCubeSamples(&MatB, MinBound, MaxBound);
	}
	else if(Method == 1)
	{
		DrawUniformSamples(&MatA, MinBound, MaxBound);
		DrawUniformSamples(&MatB, MinBound, MaxBound);
	}
	
	//NOTE This is a bit wasteful since we already allocated storage for result data in the
	//mcmc_data. But it is very convenient to have these in one large vector below ...
	std::vector<double> f0(NSamples*2);
	double *fA = f0.data();
	double *fB = fA + NSamples;
	
	int TotalEvals = NSamples*(NPars + 2);
	int Evals = 0;
	
	Array<AsyncWork<double>> Workers;
	auto NThreads = std::thread::hardware_concurrency();
	int NWorkers = std::max(32, (int)NThreads);
	Workers.InsertN(0, NWorkers);
	
	std::vector<void *> DataSets(NWorkers);
	for(int Worker = 0; Worker < NWorkers; ++Worker)
		DataSets[Worker] = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false, true);
	
	for(int SuperSample = 0; SuperSample < NSamples/NWorkers+1; ++SuperSample)
	{
		for(int Worker = 0; Worker < NWorkers; ++Worker)
		{
			int Sample = SuperSample*NWorkers + Worker;
			if(Sample >= NSamples) break;
			Workers[Worker].Do([=, & MatA, & MatB, & fA, & fB, &DataSets] () -> double {
				column_vector Pars(NPars);
				for(int Par = 0; Par < NPars; ++Par)
					Pars(Par) = MatA(0, Par, Sample);
				
				fA[Sample] = Optim->EvaluateObjectives(DataSets[Worker], Pars);
		
				for(int Par = 0; Par < NPars; ++Par)
					Pars(Par) = MatB(0, Par, Sample);
				
				fB[Sample] = Optim->EvaluateObjectives(DataSets[Worker], Pars);
				
				return 0.0; //NOTE: Just to be able to reuse the same workers. we have them returning double
			});
		}
		for(auto &Worker : Workers) Worker.Get();
		
		Evals += NWorkers*2;
		if(Evals > NSamples*2) Evals = NSamples*2;
		if(SuperSample % 8 == 0)
		{
			SensWin.ShowProgress.Set(Evals, TotalEvals);
			ParentWindow->ProcessEvents();
		}
	}
	
	int NBinsHistogram = SensWin.Plot.AddHistogram(Null, Null, f0.data(), 2*NSamples);
	Time Dummy;
	SensWin.Plot.FormatAxes(MajorMode_Histogram, NBinsHistogram, Dummy, ParentWindow->TimestepSize);
	SensWin.Plot.ShowLegend(false);
	
	//TODO: What should we use to compute overall variance?
	double mA = 0.0;
	double vA = 0.0;
	for(int J = 0; J < 2*NSamples; ++J) mA += f0[J];
	mA /= 2.0*(double)NSamples;
	for(int J = 0; J < 2*NSamples; ++J) vA += (f0[J]-mA)*(f0[J]-mA);
	vA /= 2.0*(double)NSamples;
	
	
	for(int I = 0; I < NPars; ++I)
	{
		double Main  = 0;
		double Total = 0;
		
		for(int SuperSample = 0; SuperSample < NSamples/NWorkers+1; ++SuperSample)
		{
			for(int Worker = 0; Worker < NWorkers; ++Worker)
			{
				int Sample = SuperSample*NWorkers + Worker;
				if(Sample >= NSamples) break;
				
				Workers[Worker].Do([=, & MatA, & MatB, & DataSets] () -> double {
					column_vector Pars(NPars);
					for(int II = 0; II < NPars; ++II)
					{
						if(II == I) Pars(II) = MatB(0, II, Sample);//matB[Sample*NPars + II];
						else        Pars(II) = MatA(0, II, Sample);//matA[Sample*NPars + II];
					}
					return Optim->EvaluateObjectives(DataSets[Worker], Pars);
				});
			}
			for(int Worker = 0; Worker < NWorkers; ++Worker)
			{
				int J = SuperSample*NWorkers + Worker;
				if(J >= NSamples) break;
				double fABij = Workers[Worker].Get();
				
				Main  += fB[J]*(fABij - fA[J]);
				Total += (fA[J] - fABij)*(fA[J] - fABij);
				
				Evals++;
			}
			if(SuperSample % 8 == 0)
			{
				SensWin.ShowProgress.Set(Evals, TotalEvals);
				ParentWindow->ProcessEvents();
			}
		}
		
		Main /= (vA * (double)NSamples);
		Total /= (vA * 2.0 * (double)NSamples);
		
		SensWin.ResultData.Set(I, "__main", FormatDouble(Main, ParentWindow->StatSettings.Precision));
		SensWin.ResultData.Set(I, "__total", FormatDouble(Total, ParentWindow->StatSettings.Precision));
		
		ParentWindow->ProcessEvents();
	}
	
	SensWin.ShowProgress.Hide();
	
	for(int Worker = 0; Worker < NWorkers; ++Worker)
		ParentWindow->ModelDll.DeleteDataSet(DataSets[Worker]);
	
	MatA.Free();
	MatB.Free();
	
	return true;
	
//	#undef DO_PROGRESS
}

VarianceSensitivityWindow::VarianceSensitivityWindow()
{
	CtrlLayout(*this, "MobiView variance based sensitivity results");
	MinimizeBox().Sizeable().Zoomable();

	ShowProgress.Set(0, 1);
	ShowProgress.Hide();
	
	ResultData.AddColumn("__par", "Parameter");
	ResultData.AddColumn("__main", "First-order sensitivity coefficient");
	ResultData.AddColumn("__total", "Total effect index");
	
	Plot.SetLabels("Combined statistic", "Frequency");
}

bool OptimizationWindow::ErrSymFixup()
{
	std::unordered_map<std::string, std::pair<int,int>> SymRow;
	
	//if((bool)ParSetup.OptionUseExpr.Get() || RunType==1 || RunType==2 || RunType==3)    //TODO: Non-ideal. Instead we should maybe force use exprs for MCMC RunTypes
	//{
	int ActiveIdx = 0;
	int Row = 0;
	for(indexed_parameter &Parameter : Parameters)
	{
		if(Parameter.Symbol != "")
		{
			if(SymRow.find(Parameter.Symbol) != SymRow.end())
			{
				SetError(Format("The parameter symbol %s appears twice", Parameter.Symbol.data()));
				return false;
			}
			SymRow[Parameter.Symbol] = {Row, ActiveIdx};
		}
		
		if(Parameter.Expr == "")
			++ActiveIdx;
		
		++Row;
	}
	//}
	
	//if(RunType == 1 || RunType == 2)
	//{
	for(optimization_target &Target : Targets)
	{
		Target.ErrParNum.clear();
			
		Vector<String> Symbols = Split(Target.ErrParSym.data(), ',', true);
		
		if(GetStatClass(Target) == StatClass_LogLikelihood)
		{
			for(String &Symbol : Symbols) { Symbol = TrimLeft(TrimRight(Symbol)); }
			
			#define SET_LL_SETTING(Handle, Name, NumErr) \
				else if(Target.ErrStruct == MCMCError_##Handle && Symbols.size() != NumErr) {\
					SetError(Format("The error structure %s requires %d error parameters. Only %d were provided. Provide them as a comma-separated list of parameter symbols", Name, NumErr, Symbols.size())); \
					return false; \
				}
			if(false) {}
			#include "LLSettings.h"
			#undef SET_LL_SETTING
			
			for(String &Symbol : Symbols)
			{
				std::string Sym = Symbol.ToStd();
				
				if(SymRow.find(Sym) == SymRow.end())
				{
					SetError(Format("The error parameter symbol %s is not the symbol of a parameter in the parameter list.", Sym.data()));
					return false;
				}
				
				int ParSymRow = SymRow[Sym].first;
				indexed_parameter &Par = Parameters[SymRow[Sym].first];
				if(!Par.Virtual)
				{
					SetError(Format("Only virtual parameters can be error parameters. The parameter with symbol %s is not virtual", Sym.data()));
					return false;
				}
				if(Par.Expr != "")
				{
					SetError(Format("Error parameters can not be results of expressions. The parameter with symbol %s was set as an error parameter, but also has an expression", Sym.data()));
					return false;
				}
				
				Target.ErrParNum.push_back(SymRow[Sym].second);
			}
		}
		else if(GetStatClass(Target) == StatClass_Residual && !Symbols.empty())
		{
			SetError("An error symbol was provided for a stat does not use one");
			return false;
		}
	}
	//}
	
	return true;
}

*/
void OptimizationWindow::run_clicked(int run_type)
{
	//RunType:
	//	0 = Optimizer
	//  1 = New MCMC
	//  2 = Extend MCMC
	//  3 = Variance based sensitivity
	
	//TODO
	
	/*
	set_error("");
	
	if(Parameters.empty())
	{
		SetError("At least one parameter must be added before running");
		ProcessEvents();
		return;
	}
	
	if(Targets.empty())
	{
		SetError("There must be at least one optimization target.");
		return;
	}
	
	int Cl = 1;
	if(ParentWindow->ParametersWereChangedSinceLastSave)
		Cl = PromptYesNo("The main parameter set has been edited since the last save. If run the optimizer now it will overwrite these changes. Do you still want to run the optimizer?");
	if(!Cl)
		return;
	
	bool Success = ErrSymFixup();
	if(!Success) return;
	
	char TimeStr[256];
	uint64 ResultTimesteps = ParentWindow->ModelDll.GetNextTimesteps(ParentWindow->DataSet);
	ParentWindow->ModelDll.GetNextStartDate(ParentWindow->DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);
	Time ResultEndTime = ResultStartTime;
	AdvanceTimesteps(ResultEndTime, ResultTimesteps-1, ParentWindow->TimestepSize);
	
	//NOTE: Since we haven't wired all of the background data to the edit fields, we have to
	//gather some of it here.
	bool PositiveGood;
	int Row = 0;
	for(optimization_target &Target : Targets)
	{
		target_stat_class StatClass = GetStatClass(Target);
		
		if(
			(StatClass == StatClass_Stat && (RunType==0||RunType==1||RunType==2))
		  ||(StatClass == StatClass_LogLikelihood && (RunType==3))
		  ||(StatClass == StatClass_Residual && (RunType==1||RunType==2))
		  )
		{
			SetError("The selected stat for one of the targets is not valid for this type of run.");
			return;
		}
		
		if(RunType == 0)
		{
			bool PositiveGoodThis;
			if(StatClass == StatClass_Residual)
			{
				if(false){}
				#define SET_SETTING(Handle, Name, Type)
				#define SET_RES_SETTING(Handle, Name, Type) \
					else if(Target.ResidualStat == ResidualType_##Handle) PositiveGoodThis = (Type==1);
				#include "SetStatSettings.h"
				#undef SET_SETTING
				#undef SET_RES_SETTING
			}
			else if(StatClass == StatClass_LogLikelihood)
				PositiveGoodThis = true;
			else
				assert(!"Unsupported stat type for optimization");
			
			//NOTE: We could allow people to set negative weights in order to mix different types
			//of target, but I don't see a good use case for it currently.
			
			if(Row != 0 && (PositiveGoodThis != PositiveGood))
			{
				SetError("All optimization targets have to be either minimized or maximized, no mixing is allowed.");
				return;
			}
			PositiveGood = PositiveGoodThis;
		}
		++Row;
		
		if(Target.Weight < 0.0) //NOTE: The interface should already have prevented this, but let's be safe.
		{
			SetError("Negative weights are not allowed.");
			return;
		}
		
		if(StatClass != StatClass_Stat && Target.InputName == "")
		{
			SetError(Format("Targets that compute an error need an input observed series. No comparison series were provided for result \"%s\"", Target.ResultName.data()));
			return;
		}
		
		Time Begin, End;
		StrToTime(Begin, Target.Begin.data());
		StrToTime(End, Target.End.data());
		if(    !Begin.IsValid() || IsNull(Begin) || Begin < ResultStartTime || Begin > ResultEndTime || Begin > End
			|| !End.IsValid()   || IsNull(End)   || End   < ResultStartTime || End   > ResultEndTime)
		{
			SetError("One of the targets has a begin-end interval that is not valid or does not lie within the model run interval");
			return;
		}
	}
	
	int64 RunTimeout = TargetSetup.EditTimeout.GetData();
	
	Label *ProgressLabel = nullptr;
	if(RunSetup.OptionShowProgress.Get())
		ProgressLabel = &RunSetup.ProgressLabel;
	
	void *DataSetBase = nullptr;
	if(RunType==0) DataSetBase = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false, true);
	optimization_model OptimizationModel(ParentWindow, &Parameters, &Targets, ProgressLabel, DataSetBase, RunTimeout);
	
	// Initial evaluation on the parameters given in the main dataset.
	column_vector InitialPars(OptimizationModel.FreeParCount);
	column_vector MinBound(OptimizationModel.FreeParCount);
	column_vector MaxBound(OptimizationModel.FreeParCount);
	
	int ActiveIdx = 0;
	for(int Row = 0; Row < Parameters.size(); ++Row)
	{
		if(Parameters[Row].Expr == "")
		{
			MinBound(ActiveIdx) = (double)ParSetup.ParameterView.Get(Row, Id("__min"));
			MaxBound(ActiveIdx) = (double)ParSetup.ParameterView.Get(Row, Id("__max"));
			
			if(MinBound(ActiveIdx) >= MaxBound(ActiveIdx))
			{
				SetError(Format("The min value is larger than or equal to the max value for the parameter %s", Parameters[Row].Name.data()));
				return;
			}
			
			++ActiveIdx;
		}
	}

	int ExprCount = 0;
	ActiveIdx = 0;
	for(indexed_parameter Par : Parameters)
	{
		if(Par.Expr == "")
		{
			if(Par.Virtual)
			{
				//NOTE: The best guess we have for the initial value of a virtual parameter is
				//in the middle of its range. We could add a field to allow people to fill it
				//in though.
				InitialPars(ActiveIdx) = 0.5 * (MinBound(ActiveIdx) + MaxBound(ActiveIdx));
			}
			else
			{
				std::vector<const char *> Indexes;
				for(parameter_index &Index : Par.Indexes)
					Indexes.push_back(Index.Name.data());
				
				InitialPars(ActiveIdx) = ParentWindow->ModelDll.GetParameterDouble(ParentWindow->DataSet, Par.Name.data(), (char**)Indexes.data(), Indexes.size());
			}
			if(InitialPars(ActiveIdx) < MinBound(ActiveIdx))
			{
				SetError(Format("The parameter \"%s\" %s has an initial value that is smaller than the assigned min bound", Par.Name.data(), MakeParameterIndexString(Par)));
				return;
			}
			if(InitialPars(ActiveIdx) > MaxBound(ActiveIdx))
			{
				SetError(Format("The parameter \"%s\" %s has an initial value that is larger than the assigned max bound", Par.Name.data(), MakeParameterIndexString(Par)));
				return;
			}
			
			if((RunType==1 || RunType==2 || RunType==3) && Par.Symbol == "")
			{
				SetError("You should provide a symbol for all the parameters that don't have an expression.");
				return;
			}
			
			++ActiveIdx;
		}
		else
			++ExprCount;

	}
	if(ParentWindow->CheckDllUserError()) return;
	
	//NOTE: This sets the parameters twice, but that should not be a problem
	int ErrorAtRow = SetParameters(ParentWindow->DataSet, &Parameters, InitialPars, ExprCount > 0, ParentWindow->ModelDll);
	if(ErrorAtRow != -1)
	{
		SetError(Format("Unable to parse the expression \"%s\" at row %d", Parameters[ErrorAtRow].Expr.data(), ErrorAtRow));
		return;
	}
	
	auto Begin = std::chrono::high_resolution_clock::now();
	
	double InitialScore;
	if(RunType == 0)
		InitialScore = OptimizationModel(InitialPars);
	else
	{
		void *DataSet = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false, true);
		InitialScore = OptimizationModel.EvaluateObjectives(DataSet, InitialPars);
		ParentWindow->ModelDll.DeleteDataSet(DataSet);
	}
	
	if(ParentWindow->CheckDllUserError()) return;
	
	auto End = std::chrono::high_resolution_clock::now();
	double Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Begin).count();
	
	OptimizationModel.InitialScore = InitialScore;
	OptimizationModel.BestScore    = InitialScore;
	OptimizationModel.NumEvals     = 0;
	OptimizationModel.IsMaximizing = PositiveGood;
	//NOTE: Try to update about every 4 seconds (in practice it is faster since the first run is slower):
	int UpdateStep = std::ceil(4000.0/Ms);
	if(UpdateStep <= 0) UpdateStep = 1;
	OptimizationModel.UpdateStep = UpdateStep;
		
	bool PushExtendEnabled = MCMCSetup.PushExtendRun.IsEnabled();
	
	RunSetup.PushRun.Disable();
	MCMCSetup.PushRun.Disable();
	MCMCSetup.PushExtendRun.Disable();
	SensitivitySetup.PushRun.Disable();
	
	#define END_CLEANUP() \
	RunSetup.PushRun.Enable(); \
	MCMCSetup.PushRun.Enable(); \
	if(PushExtendEnabled) MCMCSetup.PushExtendRun.Enable(); \
	SensitivitySetup.PushRun.Enable(); \
	RunSetup.ProgressLabel.SetText(""); \
	SetError("");
	
	
	if(RunType == 0)
	{
		dlib::function_evaluation InitialEval;
		InitialEval.x = InitialPars;
		InitialEval.y = PositiveGood ? InitialScore : -InitialScore;
		
		std::vector<dlib::function_evaluation> InitialEvals = {InitialEval};
		
		if(ParentWindow->CheckDllUserError())
			return;
		
		int MaxFunctionCalls = RunSetup.EditMaxEvals.GetData();
		double Epsilon       = RunSetup.EditEpsilon.GetData();

		double ExpectedDuration = Ms*1e-3*(double)MaxFunctionCalls;
			
		ParentWindow->Log(Format("Running optimization. Expected duration around %g seconds.", ExpectedDuration));
	
		SetError("Running optimization. This may take a few minutes or more.");
		
		ParentWindow->ProcessEvents();
		
		//TODO: We need to disable some interactions with the main dataset when this is running
		//(such as running the model or editing parameters!)
		
		//TODO: Something goes wrong when we capture the OptimizationModel by reference. It somehow loses the cached
		//input data inside the run... This should not happen, as the input data should be
		//cached in the first initialization run above, and not changed after that!
		
		//TODO: Threading here sometimes crashes on some machines, but not all!
		
		//TODO: Sometimes the new best score (Result.y) is reported as higher than what you get
		//when setting the parameters to be Result.x and re-running!!! What is going on there?
		//Also, sometimes the reported initial value is wrong.!!
		
		Thread().Run([=, & InitialEvals](){
			
			auto BeginTime = std::chrono::high_resolution_clock::now();
			
			std::vector<bool> Dummy;
			dlib::function_evaluation Result 
				= dlib::find_max_global(OptimizationModel, 
										MinBound, 
										MaxBound,
										dlib::max_function_calls(MaxFunctionCalls), 
										dlib::FOREVER, 
										Epsilon, 
										InitialEvals
				);
				
			auto EndTime = std::chrono::high_resolution_clock::now();
			double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
			
			double NewScore = Result.y;
			if(!PositiveGood) NewScore = -NewScore;
			
			GuiLock Lock;
			if((PositiveGood && (NewScore <= InitialScore)) || (!PositiveGood && (NewScore >= InitialScore)) || !std::isfinite(NewScore))
			{
				//ParentWindow->Log(Format("Initial %g new %g", InitialScore, NewScore));
				ParentWindow->Log("The optimizer was unable to find a better result using the given number of function evaluations");
			}
			else
			{
				SetParameters(ParentWindow->DataSet, &Parameters, Result.x, ExprCount > 0, ParentWindow->ModelDll);
				ParentWindow->Log(Format("Optimization finished after %g seconds, with new best aggregate score: %g (old: %g). Remember to save these parameters to a different file if you don't want to overwrite your old parameter set.",
					Duration, NewScore, InitialScore));
				ParentWindow->RunModel();  // We call the RunModel function of the ParentWindow instead of doing it directly on the dll so that plots etc. are updated.
			}
			
			END_CLEANUP();
		});

	}
	else if(RunType == 1 || RunType == 2)
	{
		int NWalkers    = MCMCSetup.EditWalkers.GetData();
		int NSteps      = MCMCSetup.EditSteps.GetData();
		int InitialType = MCMCSetup.InitialTypeSwitch.GetData();
		
		int NPars = OptimizationModel.FreeParCount;
		
		mcmc_sampler_method Method = (mcmc_sampler_method)(int)MCMCSetup.SelectSampler.GetData();
		double SamplerParams[20]; //NOTE: We should not encounter a sampler with more parameters than this...
		for(int Par = 0; Par < MCMCSetup.SamplerParamView.GetCount(); ++Par)
			SamplerParams[Par] = MCMCSetup.SamplerParamView.Get(Par, 1);
		
		std::vector<double> InitialVals(NPars);
		std::vector<double> MinVals(NPars);
		std::vector<double> MaxVals(NPars);
		
		for(int Idx = 0; Idx < NPars; ++Idx)
		{
			InitialVals[Idx] = InitialPars(Idx);
			MinVals[Idx]     = MinBound(Idx);
			MaxVals[Idx]     = MaxBound(Idx);
		}
		
		auto NCores = std::thread::hardware_concurrency();
		int NActualSteps = (RunType==1) ? (NSteps) : (NSteps - Data.NSteps);
		String Prefix = (RunType==1) ? "Running MCMC." : "Extending MCMC run.";
		
		double ExpectedDuration1 = Ms*1e-3*(double)NWalkers*(double)NActualSteps/(double)NCores;
		double ExpectedDuration2 = 2.0*ExpectedDuration1;   //TODO: This is just because we are not able to get the number of physical cores..
		
		ParentWindow->Log(Format("%s Expected duration around %.1f to %.1f seconds. Number of logical cores: %d.", Prefix, ExpectedDuration1, ExpectedDuration2, (int)NCores));
		ParentWindow->ProcessEvents();
		
		
		int CallbackInterval = 50; //TODO: Make this configurable or dynamic?
		
		if(!ParentWindow->MCMCResultWin.IsOpen())
			ParentWindow->MCMCResultWin.Open();
		
		//NOTE: If we launch this from a separate thread, it can't launch its own threads, and
		//the GUI doesn't update properly :(
		
		//Thread().Run([=, & OptimizationModel, & InitialVals, & MinVals, & MaxVals](){
			
			auto BeginTime = std::chrono::system_clock::now();
			
			bool Finished = RunMobiviewMCMC(Method, &SamplerParams[0], NWalkers, NSteps, &OptimizationModel, InitialVals.data(), MinVals.data(), MaxVals.data(),
											InitialType, CallbackInterval, RunType);
			
			auto EndTime = std::chrono::system_clock::now();
			double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
			
			GuiLock Lock;
			
			if(Finished)
				ParentWindow->Log(Format("MCMC run finished after %g seconds.", Duration));
			else
				ParentWindow->Log("MCMC run unsuccessful.");
			
			MCMCSetup.PushExtendRun.Enable();
			
		END_CLEANUP();
		//});
	}
	else if(RunType == 3)
	{
		int NSamples = SensitivitySetup.EditSampleSize.GetData();
		int Method   = SensitivitySetup.SelectMethod.GetData();
		
		int NPars = OptimizationModel.FreeParCount;
		std::vector<double> MinVals(NPars);
		std::vector<double> MaxVals(NPars);
		
		for(int Idx = 0; Idx < NPars; ++Idx)
		{
			MinVals[Idx]     = MinBound(Idx);
			MaxVals[Idx]     = MaxBound(Idx);
		}
		
		auto NCores = std::thread::hardware_concurrency();
		double ExpectedDuration1 = Ms*1e-3*(double)NSamples*(2.0 + (double)NPars) / (double)NCores;
		double ExpectedDuration2 = 2.0*ExpectedDuration1;   //TODO: This is just because we are not able to get the number of physical cores..
		
		ParentWindow->Log(Format("Running variance based sensitiviy sampling. Expected duration around %.1f to %.1f seconds. Number of logical cores: %d.", ExpectedDuration1, ExpectedDuration2, (int)NCores));
		
		auto BeginTime = std::chrono::system_clock::now();
		
		RunVarianceBasedSensitivity(NSamples, Method, &OptimizationModel, MinVals.data(), MaxVals.data());
		
		auto EndTime = std::chrono::system_clock::now();
		double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
		ParentWindow->Log(Format("Variance based sensitivity sampling finished after %g seconds.", Duration));
		
		END_CLEANUP();
	}
	
	#undef END_CLEANUP
	*/
}

void OptimizationWindow::tab_change() {
	//TODO:
	/*
	int TabNum = TargetSetup.OptimizerTypeTab.Get();
	
	if (TabNum == 2)             // variance based    -- hide err params
		TargetSetup.TargetView.HeaderObject().HideTab(5);
	else                         // MCMC or optimizer -- show err params
		TargetSetup.TargetView.HeaderObject().ShowTab(5);
	
	for(int TargetIdx = 0; TargetIdx < Targets.size(); ++TargetIdx)
	{
		DropList &SelectStat = TargetStatCtrls[TargetIdx];
		SelectStat.Clear();

		if(TabNum == 2)
		{
			#define SET_SETTING(Handle, Name, Type) SelectStat.Add((int)StatType_##Handle, Name);
			#define SET_RES_SETTING(Handle, Name, Type)
			#include "SetStatSettings.h"
			#undef SET_SETTING
			#undef SET_RES_SETTING
		}
		if(TabNum == 0 || TabNum == 2)
		{
			#define SET_SETTING(Handle, Name, Type)
			#define SET_RES_SETTING(Handle, Name, Type) if(Type != -1) SelectStat.Add((int)ResidualType_##Handle, Name);
			#include "SetStatSettings.h"
			#undef SET_SETTING
			#undef SET_RES_SETTING
		}
		if(TabNum == 0 || TabNum == 1)
		{
			#define SET_LL_SETTING(Handle, Name, NumErr) SelectStat.Add((int)MCMCError_##Handle, Name);
			#include "LLSettings.h"
			#undef SET_LL_SETTING
		}
		
		optimization_target &Target = Targets[TargetIdx];
		SelectStat.SetValue((int)Target.Stat);
		TargetSetup.TargetView.Set(TargetIdx, "__targetstat", (int)Target.Stat);
		//PromptOK(Format("Target stat is %d", (int)Target.Stat));
	}
	*/
}

//// Serialization:
	
void OptimizationWindow::read_from_json_string(const String &json) {
	//TODO
	/*
	ClearAll();
	
	Value SetupJson  = ParseJSON(JsonData);
	
	Value MaxEvalsJson = SetupJson["MaxEvals"];
	if(!IsNull(MaxEvalsJson))
		RunSetup.EditMaxEvals.SetData((int)MaxEvalsJson);
	
	Value EpsilonJson = SetupJson["Epsilon"];
	if(!IsNull(EpsilonJson))
		RunSetup.EditEpsilon.SetData((double)EpsilonJson);

	
	ValueArray ParameterArr = SetupJson["Parameters"];
	if(!IsNull(ParameterArr))
	{
		int Count = ParameterArr.GetCount();
		int ValidRow = 0;
		for(int Row = 0; Row < Count; ++Row)
		{
			Value ParamJson = ParameterArr[Row];
			
			indexed_parameter Parameter = {};
			Parameter.Valid = true;
			Parameter.Type  = ParameterType_Double;
			
			Value NameVal = ParamJson["Name"];
			if(!IsNull(NameVal)) Parameter.Name = NameVal.ToString().ToStd();
			else continue; // Should never happen though
			
			Value VirtualVal = ParamJson["Virtual"];
			if(!IsNull(VirtualVal)) Parameter.Virtual = (bool)VirtualVal;
			
			ValueArray IndexesVal = ParamJson["Indexes"];
			for(int Idx = 0; Idx < IndexesVal.GetCount(); ++Idx)
			{
				Value IndexJson = IndexesVal[Idx];
				parameter_index Index;
				
				Value NameVal = IndexJson["Name"];
				if(!IsNull(NameVal)) Index.Name = NameVal.ToString().ToStd();
				Value IndexSetVal = IndexJson["IndexSetName"];
				if(!IsNull(IndexSetVal)) Index.IndexSetName = IndexSetVal.ToString().ToStd();
				Value LockedVal = IndexJson["Locked"];
				if(!IsNull(LockedVal)) Index.Locked = (bool)LockedVal;
				
				Parameter.Indexes.push_back(Index);
			}
			
			Value SymVal  = ParamJson["Sym"];
			if(!IsNull(SymVal)) Parameter.Symbol = SymVal.ToStd();
			Value ExprVal = ParamJson["Expr"];
			if(!IsNull(ExprVal)) Parameter.Expr = ExprVal.ToStd();
			
			bool Added = AddSingleParameter(Parameter, 0, false);
			
			if(Added)
			{
				Value UnitVal = ParamJson["Unit"];
				ParSetup.ParameterView.Set(ValidRow, Id("__unit"), UnitVal);
				Value MinVal  = ParamJson["Min"];
				ParSetup.ParameterView.Set(ValidRow, Id("__min"), MinVal);
				Value MaxVal  = ParamJson["Max"];
				ParSetup.ParameterView.Set(ValidRow, Id("__max"), MaxVal);
				
				++ValidRow;
			}
		}
	}
	
	Value EnableExpr = SetupJson["EnableExpr"];
	if(!IsNull(EnableExpr))
		ParSetup.OptionUseExpr.Set((int)EnableExpr);
	
	Value ShowProgress = SetupJson["ShowProgress"];
	if(!IsNull(ShowProgress))
		RunSetup.OptionShowProgress.Set((int)ShowProgress);
	
	Value NWalkers = SetupJson["Walkers"];
	if(!IsNull(NWalkers))
		MCMCSetup.EditWalkers.SetData(NWalkers);
	
	Value NSteps   = SetupJson["Steps"];
	if(!IsNull(NSteps))
		MCMCSetup.EditSteps.SetData(NSteps);
	
	Value InitType = SetupJson["InitType"];
	if(!IsNull(InitType))
		MCMCSetup.InitialTypeSwitch.SetData(InitType);
	
	Value Sampler = SetupJson["Sampler"];
	if(!IsNull(Sampler))
	{
		int Key = MCMCSetup.SelectSampler.FindValue(Sampler);
		MCMCSetup.SelectSampler.SetData(Key);
	}
	SamplerMethodSelected();
	ValueArray SamplerPars = SetupJson["SamplerPars"];
	if(!IsNull(SamplerPars) && SamplerPars.GetCount() == MCMCSetup.SamplerParamView.GetCount())
	{
		for(int Par = 0; Par < SamplerPars.GetCount(); ++Par)
			MCMCSetup.SamplerParamView.Set(Par, 1, (double)SamplerPars[Par]);
	}
	
	Value NSamples = SetupJson["Samples"];
	if(!IsNull(NSamples))
		SensitivitySetup.EditSampleSize.SetData(NSamples);
	
	Value Method = SetupJson["Method"];
	if(!IsNull(Method))
	{
		int Key = SensitivitySetup.SelectMethod.FindValue(Method);
		SensitivitySetup.SelectMethod.SetData(Key);
	}
	
	Value RunType  = SetupJson["RunType"];
	if(!IsNull(RunType))
		TargetSetup.OptimizerTypeTab.Set(RunType);
	
	Value Timeout  = SetupJson["TimeoutMs"];
	if(!IsNull(Timeout))
		TargetSetup.EditTimeout.SetData(Timeout);
	
	ValueArray TargetArr = SetupJson["Targets"];
	if(!IsNull(TargetArr))
	{
		int Count = TargetArr.GetCount();
		for(int Row = 0; Row < Count; ++Row)
		{
			Value TargetJson = TargetArr[Row];
			
			optimization_target Target = {};
			
			String ResultName = TargetJson["ResultName"];
			if(!IsNull(ResultName))
				Target.ResultName = ResultName.ToStd();
			
			String InputName  = TargetJson["InputName"];
			if(!IsNull(InputName))
				Target.InputName  = InputName.ToStd();
			
			Target.Stat         = StatType_Mean;
			Target.ResidualStat = ResidualType_MAE;
			Target.ErrStruct    = MCMCError_NormalHet1;
			
			String StatName = TargetJson["Stat"];
			if(!IsNull(StatName))
			{
				if(false){}
				#define SET_SETTING(Handle, Name, Type)
				#define SET_RES_SETTING(Handle, Name, Type) else if(Name == StatName) Target.ResidualStat = ResidualType_##Handle;
				#include "SetStatSettings.h"
				#undef SET_SETTING
				#undef SET_RES_SETTING

				if(false){}
				#define SET_SETTING(Handle, Name, Type) else if(Name == StatName) Target.Stat = StatType_##Handle;
				#define SET_RES_SETTING(Handle, Name, Type)
				#include "SetStatSettings.h"
				#undef SET_SETTING
				#undef SET_RES_SETTING

				#define SET_LL_SETTING(Handle, Name, NumErr) else if(Name == StatName) Target.ErrStruct = MCMCError_##Handle;
				if(false){}
				#include "LLSettings.h"
				#undef SET_LL_SETTING
			}
			
			ValueArray ResultIndexArr = TargetJson["ResultIndexes"];
			if(!IsNull(ResultIndexArr))
			{
				int Count2 = ResultIndexArr.GetCount();
				for(int Row2 = 0; Row2 < Count2; ++Row2)
				{
					String Index = ResultIndexArr[Row2];
					Target.ResultIndexes.push_back(Index.ToStd());
				}
			}
			
			ValueArray InputIndexArr = TargetJson["InputIndexes"];
			if(!IsNull(InputIndexArr))
			{
				int Count2 = InputIndexArr.GetCount();
				for(int Row2 = 0; Row2 < Count2; ++Row2)
				{
					String Index = InputIndexArr[Row2];
					Target.InputIndexes.push_back(Index.ToStd());
				}
			}
			
			Value WeightVal = TargetJson["Weight"];
			if(!IsNull(WeightVal)) Target.Weight = (double)WeightVal;
			
			Value ErrPar = TargetJson["ErrPar"];
			if(!IsNull(ErrPar)) Target.ErrParSym = ErrPar.ToString().ToStd();
			
			Value BeginVal = TargetJson["Begin"];
			if(!IsNull(BeginVal)) Target.Begin = BeginVal.ToString().ToStd();
			
			Value EndVal   = TargetJson["End"];
			if(!IsNull(EndVal)) Target.End = EndVal.ToString().ToStd();
			
			AddOptimizationTarget(Target);
		}
	}
	
	EnableExpressionsClicked();
	*/
}

void OptimizationWindow::read_from_json() {
	//TODO
	/*
	FileSel Sel;
	
	Sel.Type("Calibration setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteOpen();
	String Filename = Sel.Get();
	
	if(!FileExists(Filename)) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store calibration setups.");
		return;
	}
	
	String JsonData = LoadFile(Filename);
	
	LoadFromJsonString(JsonData);
	*/
}

String OptimizationWindow::write_to_json_string() {
	//TODO
	/*
	Json MainFile;
	
	MainFile("MaxEvals", RunSetup.EditMaxEvals.GetData());
	
	MainFile("Epsilon", RunSetup.EditEpsilon.GetData());
	
	JsonArray ParameterArr;
	
	//TODO: Factor out a serialization method for an indexed parameter?
	int Row = 0;
	for(indexed_parameter &Par : Parameters)
	{
		Json ParJson;
		ParJson("Name", Par.Name.data());
		ParJson("Virtual", Par.Virtual);
		
		ParJson("Unit", ParSetup.ParameterView.Get(Row, Id("__unit")));
		ParJson("Min", (double)ParSetup.ParameterView.Get(Row, Id("__min")));
		ParJson("Max", (double)ParSetup.ParameterView.Get(Row, Id("__max")));
		ParJson("Sym", Par.Symbol.data());
		ParJson("Expr", Par.Expr.data());
		
		JsonArray IndexArr;
		for(parameter_index &Index : Par.Indexes)
		{
			Json IndexJson;
			IndexJson("Name", Index.Name.data());
			IndexJson("IndexSetName", Index.IndexSetName.data());
			IndexJson("Locked", Index.Locked);
			IndexArr << IndexJson;
		}
		ParJson("Indexes", IndexArr);
		
		ParameterArr << ParJson;
		
		++Row;
	}
	
	MainFile("Parameters", ParameterArr);
	
	bool EnableExpr = (bool)ParSetup.OptionUseExpr.Get();
	MainFile("EnableExpr", EnableExpr);
	
	bool ShowProgress = (bool)RunSetup.OptionShowProgress.Get();
	MainFile("ShowProgress", ShowProgress);
	
	int NWalkers = MCMCSetup.EditWalkers.GetData();
	MainFile("Walkers", NWalkers);
	
	int NSteps   = MCMCSetup.EditSteps.GetData();
	MainFile("Steps", NSteps);
	
	int InitType = MCMCSetup.InitialTypeSwitch.GetData();
	MainFile("InitType", InitType);
	
	String Sampler = MCMCSetup.SelectSampler.GetValue();
	MainFile("Sampler", Sampler);
	JsonArray SamplerPars;
	for(int Row = 0; Row < MCMCSetup.SamplerParamView.GetCount(); ++Row)
		SamplerPars << MCMCSetup.SamplerParamView.Get(Row, 1);
	MainFile("SamplerPars", SamplerPars);
	
	int RunType  = TargetSetup.OptimizerTypeTab.Get();
	MainFile("RunType", RunType);
	
	int Timeout = TargetSetup.EditTimeout.GetData();
	MainFile("TimeoutMs", Timeout);
	
	int NSamples = SensitivitySetup.EditSampleSize.GetData();
	MainFile("Samples", NSamples);
	
	String Method = SensitivitySetup.SelectMethod.GetValue();
	MainFile("Method", Method);
	
	JsonArray TargetArr;
	
	Row = 0;
	for(optimization_target &Target : Targets)
	{
		Json TargetJson;
		TargetJson("ResultName", Target.ResultName.data());
		TargetJson("InputName", Target.InputName.data());
		
		JsonArray ResultIndexArr;
		for(std::string &Index : Target.ResultIndexes)
			ResultIndexArr << Index.data();
		TargetJson("ResultIndexes", ResultIndexArr);
		
		JsonArray InputIndexArr;
		for(std::string &Index : Target.InputIndexes)
			InputIndexArr << Index.data();
		TargetJson("InputIndexes", InputIndexArr);
		
		String StatName = Null;
		#define SET_SETTING(Handle, Name, Type)
		#define SET_RES_SETTING(Handle, Name, Type) else if(Target.ResidualStat == ResidualType_##Handle) StatName = Name;
		if(false){}
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
		
		#define SET_SETTING(Handle, Name, Type) else if(Target.Stat == StatType_##Handle) StatName = Name;
		#define SET_RES_SETTING(Handle, Name, Type)
		if(false){}
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
		
		#define SET_LL_SETTING(Handle, Name, NumErr) else if(Target.ErrStruct == MCMCError_##Handle) StatName = Name;
		if(false){}
		#include "LLSettings.h"
		#undef SET_LL_SETTING
		
		//TODO: Fix this one when we make multiple error params possible
		String ErrName  = TargetSetup.TargetView.Get(Row, Id("__errparam"));
		
		TargetJson("Stat", StatName);
		TargetJson("Weight", Target.Weight);
		TargetJson("ErrPar", ErrName);
		TargetJson("Begin", Target.Begin.data());
		TargetJson("End", Target.End.data());
		
		TargetArr << TargetJson;
		
		++Row;
	}
	
	MainFile("Targets", TargetArr);
	
	return MainFile.ToString();
	*/
	return "";
}

void OptimizationWindow::write_to_json() {
	//TODO
	/*
	FileSel Sel;

	Sel.Type("Calibration setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteSaveAs();
	String Filename = Sel.Get();
	
	if(Filename.GetLength() == 0) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store calibration setups.");
		return;
	}
	
	String JsonData = SaveToJsonString();
	
	SaveFile(Filename, JsonData);
	*/
}
