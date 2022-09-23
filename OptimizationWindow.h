#ifndef _MobiView2_OptimizationWindow_h_
#define _MobiView2_OptimizationWindow_h_

#include <CtrlLib/CtrlLib.h>
#include "PlotCtrl.h"

#define LAYOUTFILE <MobiView2/OptimizationWindow.lay>
#include <CtrlCore/lay.h>

#include "support/optimization.h"


class OptimizationParameterSetup : public WithOptimizationLayout<Upp::ParentCtrl> {
public:
	typedef OptimizationParameterSetup CLASSNAME;
	OptimizationParameterSetup();
};

class OptimizationTargetSetup : public WithOptimizationTargetLayout<Upp::ParentCtrl> {
public:
	typedef OptimizationTargetSetup CLASSNAME;
	OptimizationTargetSetup();
};

class OptimizationRunSetup : public WithOptimizerSetupLayout<Upp::ParentCtrl> {
public:
	typedef OptimizationRunSetup CLASSNAME;
	OptimizationRunSetup();
};

class MCMCRunSetup : public WithMCMCSetupLayout<Upp::ParentCtrl> {
public:
	typedef MCMCRunSetup CLASSNAME;
	MCMCRunSetup();
};

class SensitivityRunSetup : public WithSensitivitySetupLayout<Upp::ParentCtrl> {
public:
	typedef SensitivityRunSetup CLASSNAME;
	SensitivityRunSetup();
};

class MobiView2;

class OptimizationWindow : public Upp::TopWindow {
public:
	typedef OptimizationWindow CLASSNAME;
	
	OptimizationWindow(MobiView2 *parent);
	
	void add_parameter_clicked();
	void add_group_clicked();
	void remove_parameter_clicked();
	void clear_parameters_clicked();
	void add_virtual_clicked();
	void enable_expressions_clicked();
	void add_target_clicked();
	void remove_target_clicked();
	void clear_targets_clicked();
	void run_clicked(int run_mode);
	void display_clicked();
	
	void clean();
	void set_error(const Upp::String &err_str);
	
	void read_from_json_string(const Upp::String &json);
	Upp::String write_to_json_string();
	
	void symbol_edited(int row);
	void expr_edited(int row);
	void err_sym_edited(int row);
	void weight_edited(int row);
	void stat_edited(int row);
	void start_edited(int row);
	void end_edited(int row);
	
	void sampler_method_selected();
	
	//bool err_sym_fixup();
	
	//TODO: This should be a general function, not a member of this class...
	//void TargetsToPlotSetups(std::vector<optimization_target> &Targets, std::vector<plot_setup> &PlotSetups);
	
private:
	
	MobiView2 *parent;
	
	
	bool add_single_parameter(Indexed_Parameter parameter);
	void add_optimization_target(Optimization_Target &target);
	
	void sub_bar(Upp::Bar &bar);
	void write_to_json();
	void read_from_json();
	
	void tab_change();
	
	//bool RunMobiviewMCMC(mcmc_sampler_method Method, double *SamplerParams, size_t NWalkers, size_t NSteps, optimization_model *OptimModel,
	//	double *InitialValue, double *MinBound, double *MaxBound, int InitialType,
	//	int CallbackInterval, int RunType);
		
		
	//bool RunVarianceBasedSensitivity(int NSamples, int Method, optimization_model *Optim, double *MinBound, double *MaxBound);

	
	Upp::Array<Upp::EditDoubleNotNull> edit_min_ctrls;
	Upp::Array<Upp::EditDoubleNotNull> edit_max_ctrls;
	Upp::Array<Upp::EditField>         edit_sym_ctrls;
	Upp::Array<Upp::EditField>         edit_expr_ctrls;

	Upp::Array<Upp::DropList>          target_stat_ctrls;
	Upp::Array<Upp::EditDoubleNotNull> target_weight_ctrls;
	Upp::Array<Upp::EditField>         target_err_ctrls;
	Upp::Array<Upp::EditTimeNotNull>   target_start_ctrls;
	Upp::Array<Upp::EditTimeNotNull>   target_end_ctrls;
	
	Upp::Array<Upp::EditDoubleNotNull> mcmc_sampler_param_ctrls;
	
	Upp::ToolBar tool;
	
	Upp::Splitter              main_vertical;
	OptimizationParameterSetup par_setup;
	OptimizationTargetSetup    target_setup;
	
public:
	OptimizationRunSetup       run_setup;
	MCMCRunSetup               mcmc_setup;
	SensitivityRunSetup        sensitivity_setup;
	
	//MCMC_Data                  mcmc_data;
	
	std::vector<Indexed_Parameter>   parameters;
	std::vector<Optimization_Target> targets;
};

#endif
