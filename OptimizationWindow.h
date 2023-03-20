#ifndef _MobiView2_OptimizationWindow_h_
#define _MobiView2_OptimizationWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <AutoScroller/AutoScroller.h>
#include "PlotCtrl.h"

#define LAYOUTFILE <MobiView2/OptimizationWindow.lay>
#include <CtrlCore/lay.h>

#include "support/optimization.h"
#include "support/mcmc.h"

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
	bool err_sym_fixup();
	
private:
	
	MobiView2 *parent;
	
	
	bool add_single_parameter(Indexed_Parameter parameter, bool lookup_default = true);
	void add_optimization_target(Optimization_Target &target);
	
	void sub_bar(Upp::Bar &bar);
	void write_to_json();
	void read_from_json();
	
	void tab_change();
	
	bool run_mcmc_from_window(MCMC_Sampler method, double *sampler_params, int n_walkers, int n_pars, int n_steps, Optimization_Model *optim,
		std::vector<double> &initial_values, std::vector<double> &min_bound, std::vector<double> &max_bound, int init_type, int callback_interval, int run_type);
		
		
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
	
	MC_Data                    mc_data;
	
	std::vector<Indexed_Parameter>   parameters;
	std::vector<Optimization_Target> targets;
	Expr_Parameters                  expr_pars;
};

class MCMCProjectionCtrl : public WithMCMCProjectionLayout<Upp::ParentCtrl> {
public:
	typedef MCMCProjectionCtrl CLASSNAME;
	MCMCProjectionCtrl();
};

class MCMCResultWindow : public WithMCMCResultLayout<Upp::TopWindow> {
public:
	typedef MCMCResultWindow CLASSNAME;
	
	MCMCResultWindow(MobiView2 *parent);
	
	MobiView2 *parent;
	
	void begin_new_plots(MC_Data &data, std::vector<double> &min_bound, std::vector<double> &max_bound, int run_type);
	void clean();
	void resize_chain_plots();
	void refresh_plots(s64 step = -1);
	
	void burnin_slider_event();
	void burnin_edit_event();
	
	void sub_bar(Upp::Bar &bar);
	
	void save_results();
	bool load_results();
	
	void map_to_main_pushed();
	void median_to_main_pushed();
	
	void generate_projections_pushed();

	//bool halt_was_pushed;
private:
	
	void refresh_result_summary(s64 step = -1);
	
	Upp::ToolBar tool;
	
	MC_Data *data = nullptr;
	//TODO: Pack these into MC_Data? Would make more sense for serialization.
	Upp::Vector<Upp::String> free_syms;
	std::vector<double>      min_bound;
	std::vector<double>      max_bound;
	
	Upp::AutoScroller chain_plot_scroller;
	Upp::AutoScroller triangle_plot_scroller;
	
	Upp::ParentCtrl view_chain_plots;
	Upp::ParentCtrl view_triangle_plots;
	Upp::ParentCtrl view_result_summary;
	MyRichView result_summary;
	Upp::Button push_write_map;
	Upp::Button push_write_median;
	
	std::vector<double> acor_times;
	std::vector<bool>   acor_below_tolerance;
	
	const int distr_resolution = 20;
	
	double burnin[2];
	std::vector<double>          burnin_plot_y;
	Upp::Array<Upp::ScatterCtrl> chain_plots;
	
	Upp::Array<Data_Source_Owns_XY> histogram_ds;
	Upp::Array<Upp::ScatterCtrl> histogram_plots;
	
	Upp::Array<Table_Data_Owns_XYZ> triangle_plot_ds;
	Upp::Array<Upp::ScatterCtrl> triangle_plots;
	
	MCMCProjectionCtrl  view_projections;
	
	Upp::AutoScroller   projection_plot_scroll;
	Upp::ParentCtrl     projection_plot_pane;
	Upp::Array<MyPlot>  projection_plots;
	
	Upp::AutoScroller   resid_plot_scroll;
	Upp::ParentCtrl     resid_plot_pane;
	Upp::Array<MyPlot>  resid_plots;
	Upp::Array<MyPlot>  resid_histograms;
	
	Upp::AutoScroller   autocorr_plot_scroll;
	Upp::ParentCtrl     autocorr_plot_pane;
	Upp::Array<MyPlot>  autocorr_plots;
	
public:
	Expr_Parameters                  expr_pars;
	std::vector<Optimization_Target> targets;
};

Plot_Setup
target_to_plot_setup(Optimization_Target &target, Model_Application *app);

#endif
