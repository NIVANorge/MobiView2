#include "OptimizationWindow.h"
#include "MobiView2.h"

#include "support/dlib_optimization.h"

/*
#ifdef _WIN32
	#include <winsock2.h>          //NOTE: For some reason dlib includes some windows headers in an order that upp's clang setup doesn't like
#endif

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
	target_setup.target_view.AddColumn(Id("__start"), "Start");
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
	
	
	mcmc_setup.push_run.WhenPush         = [this](){ run_clicked(1); };
	mcmc_setup.push_run.SetImage(IconImg4::Run());
	mcmc_setup.push_view_chains.WhenPush << [this]() { if(!this->parent->mcmc_window.IsOpen()) this->parent->mcmc_window.Open(); };
	mcmc_setup.push_view_chains.SetImage(IconImg4::ViewMorePlots());
	mcmc_setup.push_extend_run.WhenPush   = [this](){ run_clicked(2); };
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
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::affine_stretch,         "Affine stretch (recommended)");
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::affine_walk,            "Affine walk");
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::differential_evolution, "Differential evolution");
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::metropolis_hastings,    "Metropolis-Hastings (independent chains)");
	mcmc_setup.select_sampler.GoBegin();
	mcmc_setup.select_sampler.WhenAction << THISBACK(sampler_method_selected);
	sampler_method_selected(); //To set the sampler parameters for the initial selection
	
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
		par = app->model->parameters[parameter.id];
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
			if(is_valid(par->unit))
				unit    = app->model->units[par->unit]->data.to_utf8();
			sym     = par->symbol.data();
			name    = par->name.data();
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
	auto pars = parent->params.get_all_parameters();
	for(Indexed_Parameter &par : pars)
		add_single_parameter(par);
}

void OptimizationWindow::remove_parameter_clicked() {
	int sel_row = par_setup.parameter_view.GetCursor();
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
	
	auto app = parent->app;
	
	Data_Storage<double, Var_Id> *sim_data, *obs_data;
	State_Variable *var_sim, *var_obs;
	get_storage_and_var(&app->data, target.sim_id, &sim_data, &var_sim);
	String sim_index_str = make_index_string(sim_data->structure, target.indexes, target.sim_id);
	
	String obs_index_str;
	String obs_name;
	if(is_valid(target.obs_id)) {
		get_storage_and_var(&app->data, target.obs_id, &obs_data, &var_obs);
		String obs_index_str = make_index_string(obs_data->structure, target.indexes, target.obs_id);
		obs_name = var_obs->name.data();
	}
	
	target_stat_ctrls.Create<DropList>();
	DropList &sel_stat = target_stat_ctrls.Top();
	
	int tab = target_setup.optimizer_type_tab.Get();
	if(tab == 2) {
		#define SET_STATISTIC(handle, name) sel_stat.Add((int)Stat_Type::handle, name);
		#include "support/stat_types.incl"
		#undef SET_STATISTIC
	}
	if(tab == 0 || tab == 2) {
		#define SET_RESIDUAL(handle, name, type) if(type != -1) sel_stat.Add((int)Residual_Type::handle, name);
		#include "support/residual_types.incl"
		#undef SET_RESIDUAL
	}
	if(tab == 0 || tab == 1)
	{
		#define SET_LOG_LIKELIHOOD(handle, name, n_resid_par) sel_stat.Add((int)LL_Type::handle, name);
		#include "support/log_likelihood_types.incl"
		#undef SET_LOG_LIKELIHOOD
	}
	
	target_setup.target_view.Add(var_sim->name.data(), sim_index_str, obs_name, obs_index_str, target.stat_type, "", target.weight,
		convert_time(target.start), convert_time(target.end));
	
	int row = target_setup.target_view.GetCount()-1;
	int col = target_setup.target_view.GetPos(Id("__targetstat"));
	target_setup.target_view.SetCtrl(row, col, sel_stat);
	sel_stat.WhenAction << [this, row]() { stat_edited(row); };
	
	sel_stat.GoBegin();
	
	target_err_ctrls.Create<EditField>();
	EditField &err_ctrl = target_err_ctrls.Top();
	col     = target_setup.target_view.GetPos(Id("__errparam"));
	target_setup.target_view.SetCtrl(row, col, err_ctrl);
	err_ctrl.WhenAction << [this, row](){ err_sym_edited(row); };
	
	target_weight_ctrls.Create<EditDoubleNotNull>();
	EditDoubleNotNull &edit_wt = target_weight_ctrls.Top();
	edit_wt.Min(0.0);
	col     = target_setup.target_view.GetPos(Id("__weight"));
	target_setup.target_view.SetCtrl(row, col, edit_wt);
	edit_wt.WhenAction << [this, row](){ weight_edited(row); };
	
	target_start_ctrls.Create<EditTimeNotNull>();
	EditTimeNotNull &edit_start = target_start_ctrls.Top();
	col = target_setup.target_view.GetPos(Id("__start"));
	target_setup.target_view.SetCtrl(row, col, edit_start);
	edit_start.WhenAction << [this, row](){ start_edited(row); };
	
	target_end_ctrls.Create<EditTimeNotNull>();
	EditTimeNotNull &edit_end = target_end_ctrls.Top();
	col = target_setup.target_view.GetPos(Id("__end"));
	target_setup.target_view.SetCtrl(row, col, edit_end);
	edit_end.WhenAction << [this, row](){ end_edited(row); };
}

void OptimizationWindow::stat_edited(int row) {
	targets[row].stat_type = (int)target_stat_ctrls[row].GetData();
}

void OptimizationWindow::err_sym_edited(int row) {
	// NOTE: we don't update the stored err params here, instead we do it right before the run.
}

void OptimizationWindow::weight_edited(int row) {
	targets[row].weight = (double)target_setup.target_view.Get(row, "__weight");
}

void OptimizationWindow::start_edited(int row) {
	targets[row].start = convert_time((Time)target_setup.target_view.Get(row, "__start"));
}

void OptimizationWindow::end_edited(int row) {
	targets[row].end   = convert_time((Time)target_setup.target_view.Get(row, "__end"));
}

void OptimizationWindow::add_target_clicked() {
	
	Plot_Setup setup;
	parent->plotter.get_plot_setup(setup);
	
	if(setup.selected_results.size() != 1 || setup.selected_series.size() > 1) {
		set_error("Can only add a target that has a single selected result series and at most one input series.");
		return;
	}
	
	for(int idx = 0; idx < setup.selected_indexes.size(); ++idx) {
		if(setup.selected_indexes[idx].size() != 1 && setup.index_set_is_active[idx]) {
			set_error("Can only add a target with one single selected index per index set.");
			return;
		}
	}

	Optimization_Target target;
	target.sim_id = setup.selected_results[0];
	target.obs_id = setup.selected_series.empty() ? invalid_var : setup.selected_series[0];
	get_single_indexes(target.indexes, setup);
	target.weight = 1.0;
	
	int tab = target_setup.optimizer_type_tab.Get();
	//NOTE: Defaults.
	if(tab == 0) target.stat_type = (int)Residual_Type::offset + 2; // +1 would give mean error, which can't be optimized.
	//else if(tab == 1) target.stat_type = (int)LL_Type + 1;         //TODO: MCMC
	else if(tab == 2) target.stat_type = (int)Stat_Type::offset + 1;
	
	Time start_setting = parent->calib_start.GetData();
	Time end_setting   = parent->calib_end.GetData();
	Date_Time result_start = parent->app->data.get_start_date_parameter();
	Date_Time result_end   = parent->app->data.get_end_date_parameter();
	target.start = IsNull(start_setting) ? result_start : convert_time(start_setting);
	target.end   = IsNull(end_setting)   ? result_end   : convert_time(end_setting);
	
	add_optimization_target(target);
}

Plot_Setup
target_to_plot_setup(Optimization_Target &target, PlotCtrl *plotter) {
	Plot_Setup setup = {};
	setup.selected_indexes.resize(MAX_INDEX_SETS);
	setup.index_set_is_active.resize(MAX_INDEX_SETS);
	
	setup.mode = Plot_Mode::regular;
	setup.aggregation_period = Aggregation_Period::none;
	setup.scatter_inputs = true;
	
	setup.selected_results.push_back(target.sim_id);
	if(is_valid(target.obs_id))
		setup.selected_series.push_back(target.obs_id);

	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		if(is_valid(target.indexes[idx]))
			setup.selected_indexes[idx].push_back(target.indexes[idx]);
	}
	
	// TODO: not that clean to have this as a function on the Plot_Ctrl...
	plotter->register_if_index_set_is_active(setup);
	return setup;
}

void OptimizationWindow::display_clicked() {
	if(targets.empty()) {
		set_error("There are no targets to display the plots of");
		return;
	}
	
	set_error("");
	
	std::vector<Plot_Setup> plot_setups;
	for(auto &target : targets) {
		plot_setups.push_back(target_to_plot_setup(target, &parent->plotter));
	}

	//TODO: Same issue as for sensitivity view. Would like to zoom it to the area of the result
	//series only, not the entire input data span.
	parent->additional_plots.set_all(plot_setups);
	parent->open_additional_plots();
}

void OptimizationWindow::remove_target_clicked() {
	int sel_row = target_setup.target_view.GetCursor();
	if(sel_row < 0) return;
	
	target_setup.target_view.Remove(sel_row);
	targets.erase(targets.begin()+sel_row);
	target_weight_ctrls.Remove(sel_row);
	target_stat_ctrls.Remove(sel_row);
	target_err_ctrls.Remove(sel_row);
	target_start_ctrls.Remove(sel_row);
	target_end_ctrls.Remove(sel_row);
}

void OptimizationWindow::clear_targets_clicked() {
	target_setup.target_view.Clear();
	targets.clear();
	target_weight_ctrls.Clear();
	target_stat_ctrls.Clear();
	target_err_ctrls.Clear();
	target_start_ctrls.Clear();
	target_end_ctrls.Clear();
}
	
void OptimizationWindow::clean() {
	clear_parameters_clicked();
	clear_targets_clicked();
}

void OptimizationWindow::sampler_method_selected() {
	//TODO
	MCMC_Sampler method = (MCMC_Sampler)(int)mcmc_setup.select_sampler.GetData();
	
	mcmc_setup.sampler_param_view.Clear();
	mcmc_sampler_param_ctrls.Clear();
	
	switch(method) {
		case MCMC_Sampler::affine_stretch : {
			mcmc_setup.sampler_param_view.Add("a", 2.0, "Max. stretch factor");
		} break;
		
		case MCMC_Sampler::affine_walk : {
			mcmc_setup.sampler_param_view.Add("|S|", 10.0, "Size of sub-ensemble. Has to be between 2 and NParams/2.");
		} break;
		
		case MCMC_Sampler::differential_evolution : {
			mcmc_setup.sampler_param_view.Add("γ", -1.0, "Stretch factor. If negative, will be set to default of 2.38/sqrt(2*dim)");
			mcmc_setup.sampler_param_view.Add("b", 1e-3, "Max. random walk step (multiplied by |max - min| for each par.)");
			mcmc_setup.sampler_param_view.Add("CR", 1.0, "Crossover probability");
		} break;
		
		case MCMC_Sampler::metropolis_hastings : {
			mcmc_setup.sampler_param_view.Add("b", 0.02, "Std. dev. of random walk step (multiplied by |max - min| for each par.)");
		} break;
	}
	
	int rows = mcmc_setup.sampler_param_view.GetCount();
	mcmc_sampler_param_ctrls.InsertN(0, rows);
	for(int row = 0; row < rows; ++row)
		mcmc_setup.sampler_param_view.SetCtrl(row, 1, mcmc_sampler_param_ctrls[row]);
}


struct
MCMC_Run_Data {
	MC_Data                        *data;
	std::vector<Optimization_Model> optim_models;
	double *min_bound;
	double *max_bound;
};

struct
MCMC_Callback_Data {
	MobiView2        *parent_win;
	MCMCResultWindow *result_win;
};

bool
mcmc_callback(void *callback_data, int step) {
	auto *cbd = (MCMC_Callback_Data *)callback_data;

	//if(CBD->ParentWindow->MCMCResultWin.HaltWasPushed)
	//	return false;
	
	// TODO!
	
	//GuiLock lock;
	
	cbd->result_win->refresh_plots(step);
	cbd->parent_win->ProcessEvents();
	
	return true;
}

double
mcmc_ll_eval(void *run_data_0, int walker, int step) {
	MCMC_Run_Data *run_data = (MCMC_Run_Data *)run_data_0;
	MC_Data       *data     = run_data->data;
	
	std::vector<double> pars(data->n_pars);
	for(int par = 0; par < data->n_pars; ++par) {
		double val = (*data)(walker, par, step);
		
		if(val < run_data->min_bound[par] || val > run_data->max_bound[par])
			return -std::numeric_limits<double>::infinity();
		
		pars[par] = val;
	}
	
	return run_data->optim_models[walker].evaluate(pars.data());
}

bool
OptimizationWindow::run_mcmc_from_window(MCMC_Sampler method, double *sampler_params, int n_walkers, int n_pars, int n_steps, Optimization_Model *optim,
	std::vector<double> &initial_values, std::vector<double> &min_bound, std::vector<double> &max_bound, int init_type, int callback_interval, int run_type)
{
	int init_step = 0;
	
	MCMCResultWindow *result_win = &parent->mcmc_window;
	result_win->clean();
	
	{
		GuiLock lock;

		if(!result_win->IsOpen())
			result_win->Open();
		
		if(run_type == 2) { // NOTE RunType==2 means extend the previous run.
			if(mc_data.n_steps == 0) {
				set_error("Can't extend a run before the model has been run once");
				return false;
			}
			if(mc_data.n_steps >= n_steps) {
				set_error(Format("To extend the run, you must set a higher number of timesteps than previously. Previous was %d.", (int)mc_data.n_steps));
				return false;
			}
			if(mc_data.n_walkers != n_walkers) {
				set_error("To extend the run, you must have the same amount of walkers as before.");
				return false;
			}
			//TODO
			/*
			if(ResultWin->Parameters != Parameters || ResultWin->Targets != Targets)
			{
				//TODO: Could alternatively let you continue with the old parameter set.
				SetError("You can't extend the run since the parameters or targets have changed.");
				return false;
			}
			*/
			init_step = mc_data.n_steps - 1;
			
			mc_data.extend_steps(n_steps);
		} else {
			mc_data.free_data();
			mc_data.allocate(n_walkers, n_pars, n_steps);
		}
		
		std::vector<std::string> free_syms;
		for(Indexed_Parameter &par : parameters)
			if(par.expr.empty()) free_syms.push_back(par.symbol);
		
		//NOTE: These have to be cached so that we don't have to rely on the optimization window
		//not being edited (and going out of sync with the data)
		result_win->parameters = parameters;
		result_win->targets    = targets;
		
		result_win->choose_plots_tab.Set(0);
		result_win->begin_new_plots(mc_data, min_bound, max_bound, free_syms, run_type);
		
		//parent->ProcessEvents();
		
		if(run_type == 1) {
			// For a completely new run, initialize the walkers randomly
			std::mt19937_64 gen;
			for(int walker = 0; walker < n_walkers; ++walker) {
				for(int par = 0; par < n_pars; ++par) {
					double initial = initial_values[par];
					double draw = initial;
					if(init_type == 0) {
						//Initializing walkers to a gaussian ball around the initial parameter values.
						double std_dev = (max_bound[par] - min_bound[par])/400.0;   //TODO: How to choose scaling coefficient?
						std::normal_distribution<double> distr(initial, std_dev);
						
						draw = distr(gen);
						draw = std::max(min_bound[par], draw);
						draw = std::min(max_bound[par], draw);
					} else if(init_type == 1) {
						std::uniform_real_distribution<double> distr(min_bound[par], max_bound[par]);
						draw = distr(gen);
					} else
						PromptOK("Internal error, wrong intial type");
					mc_data(walker, par, 0) = draw;
				}
			}
		}
	}
	
	parent->ProcessEvents();
	
	std::vector<double> scales(n_pars);
	for(int par = 0; par < n_pars; ++par)
		scales[par] = max_bound[par] - min_bound[par];
	
	//TODO: We really only need a set of optim models that is the size of a
	//partial ensemble, which is about half of the total ensemble.
	MCMC_Run_Data run_data;
	for(int walker = 0; walker < n_walkers; ++walker) {
		Optimization_Model opt = *optim;
		opt.data = optim->data->copy(false);
		run_data.optim_models.push_back(opt);
	}
	run_data.data      = &mc_data;
	run_data.min_bound = min_bound.data();
	run_data.max_bound = max_bound.data();

	MCMC_Callback_Data callback_data;
	callback_data.parent_win = parent;
	callback_data.result_win = result_win;
	
	//TODO: We should check the sampler_params for correctness somehow!
	
	bool finished = false;
	try {
		finished =
			run_mcmc(method, sampler_params, scales.data(), mcmc_ll_eval, &run_data, mc_data, mcmc_callback, &callback_data, callback_interval, init_step);
	} catch(int) {
	}
	parent->log_warnings_and_errors();
	
	// Clean up copied data.
	for(int walker = 0; walker < n_walkers; ++walker)
		delete run_data.optim_models[walker].data;
	
	return finished;
}

/*
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

*/

bool
OptimizationWindow::err_sym_fixup() {
	std::unordered_map<std::string, std::pair<int,int>> sym_row;
	
	int active_idx = 0;
	int row = 0;
	for(Indexed_Parameter &parameter : parameters) {
		if(!parameter.symbol.empty()) {
			if(sym_row.find(parameter.symbol) != sym_row.end()) {
				set_error(Format("The parameter symbol %s appears twice", parameter.symbol.data()));
				return false;
			}
			sym_row[parameter.symbol] = {row, active_idx};
		}
		
		if(parameter.expr.empty())
			++active_idx;
		++row;
	}
	
	row = 0;
	for(Optimization_Target &target : targets) {
		target.err_par_idx.clear();
		
		String syms = target_setup.target_view.Get(row++, Id("__errparam"));
		Vector<String> symbols = Split(syms, ',', true);
		
		auto stat_class = is_stat_class(target.stat_type);
		if(stat_class == Stat_Class::log_likelihood) {
			for(String &sym : symbols) { sym = TrimLeft(TrimRight(sym)); }
			
			#define SET_LOG_LIKELIHOOD(handle, name, n_err) \
				else if((LL_Type)target.stat_type == LL_Type::handle && symbols.size() != n_err) {\
					set_error(Format("The error structure %s requires %d error parameters. Only %d were provided. Provide them as a comma-separated list of parameter symbols", name, n_err, symbols.size())); \
					return false; \
				}
			if(false) {}
			#include "support/log_likelihood_types.incl"
			#undef SET_LOG_LIKELIHOOD
			
			for(String &sym0 : symbols) {
				std::string sym = sym0.ToStd();
				
				if(sym_row.find(sym) == sym_row.end()) {
					set_error(Format("The error parameter symbol '%s' is not the symbol of a parameter in the parameter list.", sym.data()));
					return false;
				}
				int par_sym_row = sym_row[sym].first;
				Indexed_Parameter &par = parameters[sym_row[sym].first];
				if(!par.virt) {
					set_error(Format("Only virtual parameters can be error parameters. The parameter with symbol '%s' is not virtual", sym.data()));
					return false;
				}
				if(!par.expr.empty()) {
					set_error(Format("Error parameters can not be results of expressions. The parameter with symbol '%s' was set as an error parameter, but also has an expression", sym.data()));
					return false;
				}
				target.err_par_idx.push_back(sym_row[sym].second);
			}
		} else if(!symbols.empty()) {
			set_error("An error symbol was provided for a stat does not use one");
			return false;
		}
	}
	
	return true;
}


void OptimizationWindow::run_clicked(int run_type)
{
	//RunType:
	//	0 = Optimizer
	//  1 = New MCMC
	//  2 = Extend MCMC
	//  3 = Variance based sensitivity
	
	set_error("");
	
	if(parameters.empty()) {
		set_error("At least one parameter must be added before running");
		ProcessEvents();
		return;
	}
	
	if(targets.empty()) {
		set_error("There must be at least one optimization target.");
		return;
	}
	
	int cl = 1;
	if(parent->params.changed_since_last_save)
		cl = PromptYesNo("The main parameter set has been edited since the last save. If run the optimizer now it will overwrite these changes. Do you still want to run the optimizer?");
	if(!cl)
		return;
	
	bool success = err_sym_fixup();
	if(!success) return;
	
	auto app = parent->app;
	
	Model_Data *data = app->data.copy(); //TODO: more granular spec on what parts to copy.
	
	std::vector<double> initial_pars, min_vals, max_vals;
	
	int active_idx = 0;
	int row = 0;
	for(Indexed_Parameter &par : parameters) {
		if(par.expr == "") {
			double min = (double)par_setup.parameter_view.Get(row, Id("__min"));
			double max = (double)par_setup.parameter_view.Get(row, Id("__max"));
			double init;
			if(par.virt) {
				init = (max - min)*0.5;    //TODO: we should have a system for allowing you to set the initial value.
			} else {
				if(!is_valid(par.id)) { // This should technically not be possible
					set_error("Somehow we got an invalid parameter id for one of the parameters in the setup.");
					return;
				}
				s64 offset = data->parameters.structure->get_offset(par.id, par.indexes);
				Parameter_Value val = *data->parameters.get_value(offset);
				init = val.val_real;   // We already checked that the parameter was of type real when we added it.
			}
			String name = par_setup.parameter_view.Get(row, Id("__name"));
			if(min >= max) {
				set_error(Format("The parameter \"%s\" has a min value %f that is larger than or equal to the max value %f", name, min, max));
				return;
			}
			if(init < min) {
				set_error(Format("The parameter \"%s\" has an initial value %f that is smaller than the min value %f", name, init, min));
				return;
			}
			if(init > max) {
				set_error(Format("The parameter \"%s\" has an initial value %f that is larger than the max value %f", name, init, max));
				return;
			}
			if((run_type==1 || run_type==2 || run_type==3) && par.symbol == "") {
				set_error("You should provide a symbol for all the parameters that don't have an expression.");
				return;
			}
			
			min_vals.push_back(min);
			max_vals.push_back(max);
			initial_pars.push_back(init);
			
			++active_idx;
		}
		++row;
	}
	
	Date_Time start = app->data.get_start_date_parameter();
	Date_Time end   = app->data.get_end_date_parameter();
	s64 ms_timeout = target_setup.edit_timeout.GetData();
	
	s64 update_step;
	s64 *step_ptr = &update_step;
	
	Optim_Callback callback = nullptr;
	if(run_setup.option_show_progress.Get() && run_type == 0) {
		callback = [step_ptr, this](int n_evals, int n_timeouts, double initial_score, double best_score) {
			if(*step_ptr % n_evals == 0) {
				//GUILock lock;
				run_setup.progress_label.SetText(Format("Current evaluations: %d, timeouts: %d, best score: %g (initial: %g)", n_evals, n_timeouts, best_score, initial_score));
				if(parent->additional_plots.IsOpen())
					parent->additional_plots.build_all(true);
			
				parent->ProcessEvents();
			}
		};
	}
	
	try {
		Timer timer;     // TODO: callback
		Dlib_Optimization_Model opt_model(data, parameters, targets, initial_pars.data(), callback, ms_timeout);
		s64 ms = timer.get_milliseconds(); //NOTE: this is roughly how long it took to evaluate initial values.
		
		update_step = std::ceil(4000.0 / (double)ms);
		if(update_step <= 0) update_step = 20;
		
		bool push_extend_enabled = mcmc_setup.push_extend_run.IsEnabled();
		run_setup.push_run.Disable();
		mcmc_setup.push_run.Disable();
		mcmc_setup.push_extend_run.Disable();
		sensitivity_setup.push_run.Disable();
		
		if(run_type == 0) {
			
			int max_function_calls = run_setup.edit_max_evals.GetData();
			double epsilon         = run_setup.edit_epsilon.GetData();
			double expected_duration = (double)ms * 1e-3 * (double)max_function_calls;
			
			parent->log(Format("Running optimization. Expected duration around %g seconds.", expected_duration));
			set_error("Running optimization. This may take a few minutes or more.");
			parent->ProcessEvents();
			
			//TODO: We need to disable some interactions with the main dataset when this is running
			//(such as running the model or editing parameters!)
			
			//TODO: Something goes wrong when we capture the OptimizationModel by reference. It somehow loses the cached
			//input data inside the run... This should not happen, as the input data should be
			//cached in the first initialization run above, and not changed after that!
					// above comment is for MobiView 1, not checked for MobiView 2, which has a
					// significantly different definition of that struct.
			
			//TODO: Threading here sometimes crashes on some machines, but not all!
			
			//Thread().Run([=, & InitialEvals](){
			timer = Timer();
			bool was_improvement = run_optimization(opt_model, min_vals.data(), max_vals.data(), epsilon, max_function_calls);
			ms = timer.get_milliseconds();
			//});
			if(was_improvement) {
				app->data.parameters.copy_from(&data->parameters);
				app->data.results.copy_from(&data->results);
				parent->params.refresh(true);
				parent->plot_rebuild();
				parent->log(Format("Optimization finished after %g seconds, with new best aggregate score: %g (old: %g). Remember to save these parameters to a different file if you don't want to overwrite your old parameter set.",
					(double)ms*1e-3, opt_model.best_score, opt_model.initial_score));
			} else
				parent->log("The optimizer was unable to find a better result using the given number of function evaluations");
			
		} else if (run_type == 1 || run_type == 2) {
			int n_walkers   = mcmc_setup.edit_walkers.GetData();
			int n_steps     = mcmc_setup.edit_steps.GetData();
			int init_type   = mcmc_setup.initial_type_switch.GetData();
			
			int n_pars = initial_pars.size();
			
			MCMC_Sampler method = (MCMC_Sampler)(int)mcmc_setup.select_sampler.GetData();
			double sampler_params[16]; //NOTE: We should not encounter a sampler with more parameters than this...
			for(int par = 0; par < mcmc_setup.sampler_param_view.GetCount(); ++par)
				sampler_params[par] = mcmc_setup.sampler_param_view.Get(par, 1);
			
			
			auto n_cores = std::thread::hardware_concurrency();
			int n_actual_steps = (run_type == 1) ? (n_steps) : (n_steps - mc_data.n_steps);
			String prefix = (run_type == 1) ? "Running MCMC." : "Extending MCMC run.";
			
			double exp_duration1 = ms*1e-3*(double)n_walkers*(double)n_actual_steps/(double)n_cores;
			double exp_duration2 = 2.0*exp_duration1;   //TODO: This is just because we are not able to get the number of physical cores..
			
			parent->log(Format("%s Expected duration around %.1f to %.1f seconds. Number of logical cores: %d.",
				prefix, exp_duration1, exp_duration2, (int)n_cores));
			parent->ProcessEvents();
			
			
			int callback_interval = 50; //TODO: Make this configurable or dynamic?
			
			timer = Timer();
			bool finished = run_mcmc_from_window(method, &sampler_params[0], n_walkers, n_pars, n_steps, &opt_model,
				initial_pars, min_vals, max_vals,
				init_type, callback_interval, run_type);
			ms = timer.get_milliseconds();
			
			//GuiLock lock;
			if(finished)
				parent->log(Format("MCMC run finished after %g seconds.", (double)ms*1e-3));
			else
				parent->log("MCMC run was interrupted.");
			
			mcmc_setup.push_extend_run.Enable();
		} else if (run_type == 3) {
			//TODO
		}
		
		run_setup.push_run.Enable();
		mcmc_setup.push_run.Enable();
		if(push_extend_enabled) mcmc_setup.push_extend_run.Enable();
		sensitivity_setup.push_run.Enable();
		run_setup.progress_label.SetText("");
		set_error("");
	} catch(int) {}
	parent->log_warnings_and_errors(); //TODO: we should ideally print the errors in a log in this window instead!

	delete data; // Delete the copy that we ran the optimization on.
	
	Close(); //NOTE: otherwise the window gets hidden behind the main window some times. TODO: find a better way to fix it.
/*
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
	
	//TODO: To make this work we need a way for the base Mobius2 framework to serialize
	//Entity_Id and Var_Id . We can't simply use the name of the entity since it is not
	//guaranteed to be unique.
	
	/*
	clean();
	
	Value setup_json  = ParseJSON(json);
	
	Value max_eval = SetupJson["MaxEvals"];
	if(!IsNull(max_eval))
		run_setup.edit_max_evals.SetData((int)max_eval);
	
	Value eps = SetupJson["Epsilon"];
	if(!IsNull(eps))
		run_setup.edit_epsilon.SetData((double)eps);

	ValueArray par_arr = SetupJson["Parameters"];
	if(!IsNull(par_arr)) {
		int count = par_arr.GetCount();
		int valid_row = 0;
		for(int row = 0; row < count; ++row) {
			Value par_json = par_arr[row];
			
			Indexed_Parameter par = {};
			par.id = invalid_entity_id;

			Value name = par_json["Name"];
			if(!IsNull(name)) {
				par.id = parent->app->model.find
			}
			
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
	FileSel sel;
	
	sel.Type("Calibration setups", "*.json");
	
	if(!parent->data_file.empty())
		sel.ActiveDir(GetFileFolder(parent->data_file.data()));
	else
		sel.ActiveDir(GetCurrentDirectory());
	
	sel.ExecuteOpen();
	String file_name = sel.Get();
	
	if(!FileExists(file_name)) return;
	
	if(GetFileName(file_name) == "settings.json") {
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store calibration setups.");
		return;
	}
	
	String json = LoadFile(file_name);
	
	read_from_json_string(json);
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
