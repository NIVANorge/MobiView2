#include "OptimizationWindow.h"
#include "MobiView2.h"

#include "support/dlib_optimization.h"
#include "support/effect_indexes.h"

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
	par_setup.parameter_view.AddColumn(Id("__idx"), "Index");
	
	par_setup.parameter_view.Moving(true);
	
	par_setup.option_use_expr.Set((int)false);
	par_setup.option_use_expr.WhenAction     = THISBACK(enable_expressions_clicked);
	par_setup.parameter_view.HeaderObject().HideTab(5);
	par_setup.parameter_view.HeaderObject().HideTab(6);
	par_setup.parameter_view.HeaderObject().HideTab(7);
	
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
	
	run_setup.option_show_progress.SetData(true);
	
	
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
	
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::affine_stretch,         "Affine stretch (recommended)");
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::affine_walk,            "Affine walk");
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::differential_evolution, "Differential evolution");
	mcmc_setup.select_sampler.Add((int)MCMC_Sampler::metropolis_hastings,    "Metropolis-Hastings (independent chains)");
	mcmc_setup.select_sampler.GoBegin();
	mcmc_setup.select_sampler.WhenAction << THISBACK(sampler_method_selected);
	sampler_method_selected(); //To set the sampler parameters for the initial selection
	
	sensitivity_setup.edit_sample_size.Min(10);
	sensitivity_setup.edit_sample_size.SetData(1000);
	sensitivity_setup.push_run.WhenPush  = [this](){ run_clicked(3); };

	sensitivity_setup.push_view_results.WhenPush = [this]() {	if(!this->parent->variance_window.IsOpen()) this->parent->variance_window.Open(); };
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
	
	auto font = GetStdFont();
	target_setup.error_label.SetFont(font.Bold());
	target_setup.error_label.SetInk(Red());
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

bool OptimizationWindow::add_single_parameter(Indexed_Parameter parameter, bool lookup_default) {
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

	String index_str = "";
	if(!parameter.virt)
		index_str = make_parameter_index_string(&app->parameter_structure, &parameter);
	
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
			name    = par->name.data();
			if(lookup_default)
				sym     = app->model->get_symbol(parameter.id).data();
		}
		
		parameter.symbol = sym.ToStd();
		parameters.push_back(std::move(parameter));
		//int row = parameters.size()-1;
		int idx = parameters.size()-1;
		int row = par_setup.parameter_view.GetCount();
		
		par_setup.parameter_view.Add(name, index_str, min, max, unit, sym, expr, idx);

		edit_min_ctrls.Create<EditDoubleNotNull>();
		edit_max_ctrls.Create<EditDoubleNotNull>();
		par_setup.parameter_view.SetCtrl(row, 2, edit_min_ctrls.Top());
		par_setup.parameter_view.SetCtrl(row, 3, edit_max_ctrls.Top());
		
		edit_sym_ctrls.Create<EditField>();
		edit_expr_ctrls.Create<EditField>();
		par_setup.parameter_view.SetCtrl(row, 5, edit_sym_ctrls.Top());
		par_setup.parameter_view.SetCtrl(row, 6, edit_expr_ctrls.Top());
		
		edit_sym_ctrls.Top().WhenAction <<  [this, row](){ symbol_edited(row); };
		edit_expr_ctrls.Top().WhenAction << [this, row](){ expr_edited(row); };
	}
	else
		par_setup.parameter_view.Set(overwrite_row, 1, index_str);
	
	return true;
}

void OptimizationWindow::symbol_edited(int row) {
	int idx = par_setup.parameter_view.Get(row, "__idx");
	parameters[idx].symbol = par_setup.parameter_view.Get(row, "__sym").ToStd();
}

void OptimizationWindow::expr_edited(int row) {
	int idx = par_setup.parameter_view.Get(row, "__idx");
	parameters[idx].expr = par_setup.parameter_view.Get(row, "__expr").ToStd();
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
	
	//int idx = par_setup.parameter_view.Get(sel_row, "__idx");
	
	par_setup.parameter_view.Remove(sel_row);
	//parameters.erase(parameters.begin()+idx);    // NOTE: We can't do this, because then we
	// invalidate the indexes that are stored in the row for the rest of them...
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
	
	//get_storage_and_var(&app->data, target.sim_id, &sim_data, &var_sim);
	auto *sim_data = &app->data.get_storage(target.sim_id.type);
	auto var_sim = app->vars[target.sim_id];
	
	String sim_index_str = make_index_string(sim_data->structure, target.indexes, target.sim_id);
	
	Data_Storage<double, Var_Id> *obs_data;
	State_Var *var_obs;
	String obs_index_str;
	String obs_name;
	if(is_valid(target.obs_id)) {
		//get_storage_and_var(&app->data, target.obs_id, &obs_data, &var_obs);
		obs_data = &app->data.get_storage(target.obs_id.type);
		var_obs = app->vars[target.obs_id];
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

	if(target.stat_type >= 0 && target.stat_type <= (int)LL_Type::end)
		sel_stat.SetData(target.stat_type);
	else
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
	//PromptOK(Format("Set stat to %d", targets[row].stat_type));
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
target_to_plot_setup(Optimization_Target &target, Model_Application *app) {
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
	register_if_index_set_is_active(setup, app);
	return setup;
}

void OptimizationWindow::display_clicked() {
	if(targets.empty()) {
		set_error("There are no targets to display the plots of");
		return;
	}
	if(!parent->model_is_loaded()) return;
	
	set_error("");
	
	std::vector<Plot_Setup> plot_setups;
	for(auto &target : targets)
		plot_setups.push_back(target_to_plot_setup(target, parent->app));
	
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
	
	return run_data->optim_models[walker].evaluate(pars);
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
			if(ResultWin->Parameters != Parameters || ResultWin->Targets != Targets) {
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
		
		//NOTE: These have to be cached so that we don't have to rely on the optimization window
		//not being edited (and going out of sync with the data)
		result_win->expr_pars.copy(expr_pars);
		result_win->targets    = targets;
		
		result_win->choose_plots_tab.Set(0);
		result_win->begin_new_plots(mc_data, min_bound, max_bound, run_type);
		
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

struct Sensitivity_Target_State {
	std::vector<Optimization_Model> optim_models;
};

double
sensitivity_target_fun(void *state, int worker, const std::vector<double> &values) {
	auto target_state = static_cast<Sensitivity_Target_State *>(state);
	
	double result = target_state->optim_models[worker].evaluate(values);
	
	//if(worker == 0)
	//	warning_print(result, "\n");
	
	return result;
}

struct Sensitivity_Callback_State {
	VarianceSensitivityWindow *result_window;
	MobiView2                 *parent;
};

bool
sensitivity_callback(void *state, int cb_type, int par_or_sample, double main_ei, double total_ei) {
	
	auto callback_state = static_cast<Sensitivity_Callback_State *>(state);
	
	if(cb_type == 0) {
		auto &sp = callback_state->result_window->show_progress;
		sp.Set(par_or_sample);
	} else {
		double prec = callback_state->parent->stat_settings.display_settings.decimal_precision;
		int par = par_or_sample;
		
		callback_state->result_window->result_data.Set(par, "__main", FormatDouble(main_ei, prec));
		callback_state->result_window->result_data.Set(par, "__total", FormatDouble(total_ei, prec));
	}
	callback_state->parent->ProcessEvents();
	
	return true;
}

bool
OptimizationWindow::run_variance_based_sensitivity(int n_samples, int sample_method, Optimization_Model *optim, double *min_bound, double *max_bound) {
	
	int n_pars = optim->initial_pars.size();
	
	auto n_threads = std::thread::hardware_concurrency();
	int n_workers = std::max(32, (int)n_threads);
	
	Sensitivity_Target_State target_state;
	for(int worker = 0; worker < n_workers; ++worker) {
		Optimization_Model opt = *optim;
		opt.data = optim->data->copy(false);
		target_state.optim_models.push_back(opt);
	}
	
	auto win = &parent->variance_window;
	
	win->clean();
	
	Sensitivity_Callback_State callback_state;
	callback_state.result_window = win;
	callback_state.parent        = parent;
	
	int callback_interval = 10*n_workers; //TODO: Make the caller set this. Note, it must be a multiple of 2*n_workers
	
	int total_samples = n_samples*(2 + n_pars);
	win->show_progress.Set(0, total_samples);
	win->show_progress.Show();
	
	// TODO: Actually need one more type of callback for just regular progress update.
	
	int idx = 0;
	for(Indexed_Parameter &par : optim->parameters->parameters) {
		if(!optim->parameters->exprs[idx])
			win->result_data.Add(par.symbol.data(), Null, Null);
	}
	parent->ProcessEvents();
	
	std::vector<double> samples;
	compute_effect_indexes(n_samples, n_pars, n_workers, sample_method, min_bound, max_bound, 
		sensitivity_target_fun, &target_state, sensitivity_callback, &callback_state, callback_interval, samples);
	
	// Clean up copied data.
	for(int worker = 0; worker < n_workers; ++worker)
		delete target_state.optim_models[worker].data;
	
	std::vector<double> xs(samples.size());
	
	win->plot.series_data.Create<Data_Source_Owns_XY>(&xs, &samples);
	auto *data = &win->plot.series_data.Top();
	int n_bins_histogram = add_histogram(&win->plot, data, data->MinY(), data->MaxY(), data->GetCount(), Null, Null, win->plot.colors.next());
	
	format_axes(&win->plot, Plot_Mode::histogram, n_bins_histogram, {}, {});
	win->plot.ShowLegend(false);
	win->plot.Refresh();
	win->show_progress.Hide();
	
	//TODO: Maybe error handling
	
	return true;
}

VarianceSensitivityWindow::VarianceSensitivityWindow() {
	
	CtrlLayout(*this, "MobiView2 variance based sensitivity results");
	MinimizeBox().Sizeable().Zoomable();

	show_progress.Set(0, 1);
	show_progress.Hide();
	
	result_data.AddColumn("__par", "Parameter");
	result_data.AddColumn("__main", "First-order sensitivity coefficient");
	result_data.AddColumn("__total", "Total effect index");
	
	plot.SetLabels("Combined statistic", "Frequency");
}

void
VarianceSensitivityWindow::clean() {
	result_data.Clear();
	plot.clean();
}

bool
OptimizationWindow::err_sym_fixup() {
	std::unordered_map<std::string, std::pair<int,int>> sym_row;
	
	// TODO: Should not give error if "Enable expressions" is not checked.
	int active_idx = 0;
	for(int row = 0; row < expr_pars.parameters.size(); ++row) {
		auto &parameter = expr_pars.parameters[row];
		if(!parameter.symbol.empty()) {
			if(sym_row.find(parameter.symbol) != sym_row.end()) {
				set_error(Format("The parameter symbol %s appears twice", parameter.symbol.data()));
				return false;
			}
			sym_row[parameter.symbol] = {row, active_idx};
		}
		
		if(!expr_pars.exprs[row].get())
			++active_idx;
	}
	
	int row = 0;
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
				Indexed_Parameter &par = expr_pars.parameters[par_sym_row];
				if(!par.virt) {
					set_error(Format("Only virtual parameters can be error parameters. The parameter with symbol '%s' is not virtual", sym.data()));
					return false;
				}
				if(expr_pars.exprs[par_sym_row].get()) {
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
	
	int n_rows = par_setup.parameter_view.GetCount();
	
	if(!n_rows) {
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
	
	auto app = parent->app;
	
	std::vector<Indexed_Parameter> sorted_params;
	for(int row = 0; row < n_rows; ++row) {
		int idx = par_setup.parameter_view.Get(row, "__idx");
		sorted_params.push_back(parameters[idx]);
	}
	
	bool error = false;
	try {
		// NOTE: Must set the expr_pars before we call err_sym_fixup() !
		expr_pars.set(app, sorted_params);
	} catch (int) {
		set_error("There was an error. See the main log window.");
		error = true;
	}
	parent->log_warnings_and_errors();
	if(error) return;
	
	bool success = err_sym_fixup();
	if(!success) return;

	Model_Data *data = app->data.copy(); //TODO: more granular spec on what parts to copy.
	
	std::vector<double> initial_pars, min_vals, max_vals;
	
	int active_idx = 0;
	for(int row = 0; row < n_rows; ++row) {
		
		if(expr_pars.exprs[row].get()) continue; // Don't check for parameters that have expressions.
		
		auto &par = expr_pars.parameters[row];
		
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
	
	Date_Time start = app->data.get_start_date_parameter();
	Date_Time end   = app->data.get_end_date_parameter();
	s64 ms_timeout = target_setup.edit_timeout.GetData();
	
	s64 update_step = 20;
	//s64 *step_ptr = &update_step;
	
	Optim_Callback callback = nullptr;
	if(run_setup.option_show_progress.Get() && run_type == 0) {
		callback = [&update_step, this](int n_evals, int n_timeouts, double initial_score, double best_score) {
			if((n_evals % update_step) == 0) {
				//GUILock lock;
				run_setup.progress_label.SetText(Format("Current evaluations: %d, timeouts: %d, best score: %g (initial: %g)", n_evals, n_timeouts, best_score, initial_score));
				if(parent->additional_plots.IsOpen())
					parent->additional_plots.build_all(true);
			
				parent->ProcessEvents();
			}
		};
	}
	
	try {
		Timer timer;
		Dlib_Optimization_Model opt_model(data, expr_pars, targets, &initial_pars, callback, ms_timeout);
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
			int n_samples = sensitivity_setup.edit_sample_size.GetData();
			int sample_method = sensitivity_setup.select_method.GetData();
			
			int n_pars = opt_model.initial_pars.size();
			
			auto n_cores = std::thread::hardware_concurrency();
			double exp_duration_1 = ms*1e-3*(double)n_samples*(2.0 + (double)n_pars) / (double)n_cores;
			double exp_duration_2 = 2.0*exp_duration_1;   //TODO: This is just because we are not able to get the number of physical cores..
			
			parent->log(Format("Running variance based sensitiviy sampling. Expected duration around %.1f to %.1f seconds. Number of logical cores: %d.", exp_duration_1, exp_duration_2, (int)n_cores));
			
			if(!parent->variance_window.IsOpen())
				parent->variance_window.Open();
			
			timer = Timer();
			run_variance_based_sensitivity(n_samples, sample_method, &opt_model, min_vals.data(), max_vals.data());
			
			ms = timer.get_milliseconds();
			
			parent->log(Format("Variance based sensitivity sampling finished after %g seconds.", (double)ms*1e-3));
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
	
	TopMost(true, false); //NOTE: otherwise the window gets hidden behind the main window some times.
/*
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
	
	int tab = target_setup.optimizer_type_tab.Get();
	
	if (tab == 2)             // variance based    -- hide err params
		target_setup.target_view.HeaderObject().HideTab(5);
	else                         // MCMC or optimizer -- show err params
		target_setup.target_view.HeaderObject().ShowTab(5);
	
	for(int idx = 0; idx < targets.size(); ++idx) {
		DropList &sel_stat = target_stat_ctrls[idx];
		sel_stat.Clear();

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
		if(tab == 0 || tab == 1) {
			#define SET_LOG_LIKELIHOOD(handle, name, n_err) sel_stat.Add((int)LL_Type::handle, name);
			#include "support/log_likelihood_types.incl"
			#undef SET_LOG_LIKELIHOOD
		}
		
		Optimization_Target &target = targets[idx];
		sel_stat.SetValue((int)target.stat_type);
		target_setup.target_view.Set(idx, "__targetstat", (int)target.stat_type);
	}
}

//// Serialization:
	
void OptimizationWindow::read_from_json_string(const String &json) {
	
	//TODO: To make this work we need a way for the base Mobius2 framework to serialize
	//Entity_Id and Var_Id . We can't simply use the name of the entity since it is not
	//guaranteed to be unique.
	
	auto app   = parent->app;
	auto model = app->model;
	
	clean();
	
	Value setup_json  = ParseJSON(json);
	
	Value max_eval = setup_json["MaxEvals"];
	if(!IsNull(max_eval))
		run_setup.edit_max_evals.SetData((int)max_eval);
	
	Value eps = setup_json["Epsilon"];
	if(!IsNull(eps))
		run_setup.edit_epsilon.SetData((double)eps);

	ValueArray par_arr = setup_json["Parameters"];
	if(!IsNull(par_arr)) {
		int count = par_arr.GetCount();
		int valid_row = 0;
		for(int row = 0; row < count; ++row) {
			Value par_json = par_arr[row];
			
			Indexed_Parameter par = {};
			par.id = invalid_entity_id;

			// TODO: In the end it may not be sufficient to store parameters by name, they may
			// have to be scoped to par group + module!
			Value name = par_json["Name"];
			if(!IsNull(name))
				//par.id = model->parameters.find_by_name(name.ToStd());
				par.id = model->deserialize(name.ToStd(), Reg_Type::parameter);
			
			Value virt_val = par_json["Virtual"];
			if(!IsNull(virt_val)) par.virt = (bool)virt_val;
			
			ValueArray idxs_val = par_json["Indexes"];
			for(int idx = 0; idx < idxs_val.GetCount(); ++idx) {
				Value idx_val = idxs_val[idx];
				Index_T   index = invalid_index;
				Entity_Id index_set;
				
				// TODO: May need better error handling here:
				Value idx_set_name_val = idx_val["IndexSetName"];
				if(!IsNull(idx_set_name_val)) index_set = model->deserialize(idx_set_name_val.ToStd(), Reg_Type::index_set);
				Value name_val = idx_val["Name"];
				if(!IsNull(name_val) && name_val != "") {
					index = parent->app->get_index(index_set, name_val.ToStd());
					if(!is_valid(index)) {
						set_error("The stored optimization setup was incompatible with the currently loaded model and data set.");
						parent->log("The stored optimization setup was incompatible with the currently loaded model and data set.", true);
						return;
					}
				}
				Value locked_val = idx_val["Locked"];
				bool locked = false;
				if(!IsNull(locked_val)) locked = (bool)locked_val;
				
				par.indexes.push_back(index);
				par.locks.push_back(locked);
			}
			
			Value sym_val  = par_json["Sym"];
			if(!IsNull(sym_val)) par.symbol = sym_val.ToStd();
			Value expr_val = par_json["Expr"];
			if(!IsNull(expr_val)) par.expr = expr_val.ToStd();
			
			bool added = add_single_parameter(par, false);
			
			if(added) {
				Value min_val  = par_json["Min"];
				par_setup.parameter_view.Set(valid_row, Id("__min"), min_val);
				Value max_val  = par_json["Max"];
				par_setup.parameter_view.Set(valid_row, Id("__max"), max_val);
				++valid_row;
			}
		}
	}
	
	Value enable_expr = setup_json["EnableExpr"];
	if(!IsNull(enable_expr))
		par_setup.option_use_expr.Set((int)enable_expr);
	
	Value show_progress = setup_json["ShowProgress"];
	if(!IsNull(show_progress))
		run_setup.option_show_progress.Set((int)show_progress);
	
	Value n_walkers = setup_json["Walkers"];
	if(!IsNull(n_walkers))
		mcmc_setup.edit_walkers.SetData(n_walkers);
	
	Value n_steps   = setup_json["Steps"];
	if(!IsNull(n_steps))
		mcmc_setup.edit_steps.SetData(n_steps);
	
	Value init_type = setup_json["InitType"];
	if(!IsNull(init_type))
		mcmc_setup.initial_type_switch.SetData(init_type);
	
	Value sampler = setup_json["Sampler"];
	if(!IsNull(sampler)) {
		int key = mcmc_setup.select_sampler.FindValue(sampler);
		mcmc_setup.select_sampler.SetData(key);
	}
	sampler_method_selected();
	ValueArray sampler_pars = setup_json["SamplerPars"];
	if(!IsNull(sampler_pars) && sampler_pars.GetCount() == mcmc_setup.sampler_param_view.GetCount()) {
		for(int par = 0; par < sampler_pars.GetCount(); ++par)
			mcmc_setup.sampler_param_view.Set(par, 1, (double)sampler_pars[par]);
	}
	
	Value n_samples = setup_json["Samples"];
	if(!IsNull(n_samples))
		sensitivity_setup.edit_sample_size.SetData(n_samples);
	
	Value method = setup_json["Method"];
	if(!IsNull(method)) {
		int key = sensitivity_setup.select_method.FindValue(method);
		sensitivity_setup.select_method.SetData(key);
	}
	
	Value run_type  = setup_json["RunType"];
	if(!IsNull(run_type))
		target_setup.optimizer_type_tab.Set(run_type);
	
	Value timeout  = setup_json["TimeoutMs"];
	if(!IsNull(timeout))
		target_setup.edit_timeout.SetData(timeout);
	
	ValueArray target_arr = setup_json["Targets"];
	if(!IsNull(target_arr)) {
		int count = target_arr.GetCount();
		for(int row = 0; row < count; ++row) {
			Value target_json = target_arr[row];
			
			Optimization_Target target = {};
			
			String result_name = target_json["ResultName"];
			if(!IsNull(result_name))
				target.sim_id  = app->deserialize(result_name.ToStd());
			// TODO: Check that this is actually a model result, not a series
			
			String input_name  = target_json["InputName"];
			if(!IsNull(input_name)) {
				target.obs_id  = app->deserialize(input_name.ToStd());
				// TODO: Check that this is actually a series.
			}
			
			target.stat_type    = (int)Stat_Type::mean;
			
			//TODO: This doesn't seem to always work.
			String stat_name = target_json["Stat"];
			if(!IsNull(stat_name))
				target.stat_type = stat_type_from_name(stat_name.ToStd());
			
			Value weight_val = target_json["Weight"];
			if(!IsNull(weight_val)) target.weight = (double)weight_val;
			
			//TODO: this won't work for Time values, only for the Date part!
			String begin_val = target_json["Begin"];
			if(!IsNull(begin_val)) {
				Time begin;
				StrToDate(begin, begin_val);
				target.start = convert_time(begin);
			}
			
			String end_val   = target_json["End"];
			if(!IsNull(end_val)) {
				Time end;
				StrToDate(end, end_val);
				target.end   = convert_time(end);
			}
			
			ValueArray index_arr = target_json["Indexes"];
			bool found = !IsNull(index_arr) && (index_arr.GetCount() >= model->index_sets.count());
			int row2 = 0;
			target.indexes.resize(MAX_INDEX_SETS, invalid_index);
			
			for(Entity_Id index_set : model->index_sets) {
				String index_name = Null;
				if(found) index_name = index_arr[row2];
				Index_T index;
				if(!IsNull(index_name) && index_name != "")
					index = app->get_index(index_set, index_name.ToStd());
				else
					index = { index_set, -1 };
				//target.indexes.push_back(index);
				target.indexes[row2] = index;
				++row2;
			}
			
			target.set_offsets(&parent->app->data);
			add_optimization_target(target);
			
			// TODO: set these to the array instead
			String err_par = target_json["ErrPar"];
			if(!IsNull(err_par))
				target_setup.target_view.Set(row, Id("__errparam"), err_par);
		}
	}
	
	enable_expressions_clicked();
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
	
	auto app   = parent->app;
	auto model = app->model;
	
	Json main_file;
	
	main_file("MaxEvals", run_setup.edit_max_evals.GetData());
	main_file("Epsilon", run_setup.edit_epsilon.GetData());
	JsonArray parameter_arr;
	
	//TODO: Factor out a serialization method for an indexed parameter?
	int n_rows = par_setup.parameter_view.GetCount();
	for(int row = 0; row < n_rows; ++row) {
		
		int idx = par_setup.parameter_view.Get(row, Id("__idx"));
		auto &par = parameters[idx];
		
		Json par_json;
		
		if(!par.virt) {
			//auto par_data = model->parameters[par.id];
			par_json("Name", model->serialize(par.id).data());
		}
		
		par_json("Virtual", par.virt);
		
		par_json("Min", (double)par_setup.parameter_view.Get(row, Id("__min")));
		par_json("Max", (double)par_setup.parameter_view.Get(row, Id("__max")));
		par_json("Sym", par.symbol.data());
		par_json("Expr", par.expr.data());
		
		JsonArray index_arr;
		int idx2 = 0;
		for(Index_T index : par.indexes) {
			Json index_json;
			if(is_valid(index)) {
				index_json("Name", app->get_index_name(index).data());
				index_json("IndexSetName", model->index_sets[index.index_set]->name.data());
				index_json("Locked", (bool)par.locks[idx2]);
				index_arr << index_json;
			}
			++idx2;
		}
		par_json("Indexes", index_arr);
		parameter_arr << par_json;
	}
	main_file("Parameters", parameter_arr);
	
	bool enable_expr = (bool)par_setup.option_use_expr.Get();
	main_file("EnableExpr", enable_expr);
	
	bool show_progress = (bool)run_setup.option_show_progress.Get();
	main_file("ShowProgress", show_progress);
	
	int n_walkers = mcmc_setup.edit_walkers.GetData();
	main_file("Walkers", n_walkers);
	
	int n_steps   = mcmc_setup.edit_steps.GetData();
	main_file("Steps", n_steps);
	
	int init_type = mcmc_setup.initial_type_switch.GetData();
	main_file("InitType", init_type);
	
	String sampler = mcmc_setup.select_sampler.GetValue();
	main_file("Sampler", sampler);
	JsonArray sampler_pars;
	for(int row = 0; row < mcmc_setup.sampler_param_view.GetCount(); ++row)
		sampler_pars << mcmc_setup.sampler_param_view.Get(row, 1);
	main_file("SamplerPars", sampler_pars);
	
	int run_type  = target_setup.optimizer_type_tab.Get();
	main_file("RunType", run_type);
	
	int timeout = target_setup.edit_timeout.GetData();
	main_file("TimeoutMs", timeout);
	
	int n_samples = sensitivity_setup.edit_sample_size.GetData();
	main_file("Samples", n_samples);
	
	String method = sensitivity_setup.select_method.GetValue();
	main_file("Method", method);
	
	JsonArray target_arr;
	
	int row = 0;
	for(Optimization_Target &target : targets) {
		Json target_json;
		target_json("ResultName", app->serialize(target.sim_id).data());
		target_json("InputName", app->serialize(target.obs_id).data());
		
		JsonArray index_arr;
		for(Index_T index : target.indexes) {
			if(is_valid(index))
				index_arr << app->get_index_name(index).data();
			else
				index_arr << "";
		}
		
		target_json("Indexes", index_arr);
		
		std::string statname = stat_name(target.stat_type);
		//PromptOK(Format("Stat is %d %s", target.stat_type, statname.data()));

		String err_name = target_setup.target_view.Get(row, Id("__errparam"));
		target_json("Stat", statname.data());
		target_json("Weight", target.weight);
		target_json("ErrPar", err_name);
		target_json("Begin", target.start.to_string().data);
		target_json("End", target.end.to_string().data);
		
		target_arr << target_json;
		++row;
	}
	
	main_file("Targets", target_arr);
	
	return main_file.ToString();
}

void OptimizationWindow::write_to_json() {

	FileSel sel;

	sel.Type("Calibration setups", "*.json");
	
	if(!parent->data_file.empty())
		sel.ActiveDir(GetFileFolder(parent->data_file.data()));
	else
		sel.ActiveDir(GetCurrentDirectory());
	
	sel.ExecuteSaveAs();
	String file_name = sel.Get();
	if(file_name.GetLength() == 0) return;
	
	if(GetFileName(file_name) == "settings.json") {
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store calibration setups.");
		return;
	}
	
	String json_data = write_to_json_string();
	SaveFile(file_name, json_data);
}
