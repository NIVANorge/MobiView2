
#include "MobiView2.h"

#define IMAGECLASS IconImg
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

#define IMAGECLASS MainIconImg
#define IMAGEFILE <MobiView2/MobiView2.iml>
#include <Draw/iml.h>

#include <iomanip>

std::stringstream global_error_stream;
std::stringstream global_log_stream;
bool allow_logging = true;


using namespace Upp;


void
MyRichView::append(const Upp::String &to_append) {
	if(progress_bar_pos >= 0) finish_progress_bar();
	
	Upp::String data = GetQTF();
	data += to_append;
	SetQTF(data);
	ScrollEnd();
}
	
void
MyRichView::add_progress_bar() {
	// We should probably instead store a std::string representation of the data in the
	// MyRichView class and operate on that, then just SetQTF with it each time instead of
	// having to call GetQTF()
	
	Upp::String data = GetQTF();
	data += "Progress: ";
	progress_bar_pos = strlen((const char *)(data))-2;  // Oops very hacky.
	data += "`_`_`_`_`_`_`_`_`_`_&&";
	SetQTF(data);
	ScrollEnd();
	prev_progress = 0;
}

void
MyRichView::set_progress(double percent) {
	if(progress_bar_pos < 0) return;
		
	Upp::String data = GetQTF();
	int pos = (int)(percent / 10.0);
	for(int i = prev_progress; i < pos; ++i) {
		// Oops, very hacky.
		((char *)(const char *)(data))[progress_bar_pos + i*2] = '`';
		((char *)(const char *)(data))[progress_bar_pos + i*2 + 1] = '*';
	}
	prev_progress = pos;
	SetQTF(data);
	ScrollEnd();
}

void
MyRichView::finish_progress_bar() {
	progress_bar_pos = -1;
}



MobiView2::MobiView2() :
	params(this), plotter(this), stat_settings(this), search_window(this),
	sensitivity_window(this), info_window(this), additional_plots(this),
	optimization_window(this), mcmc_window(this), structure_window(this),
	model_chart_window(this),
	result_selecter(this, "Model result series", Var_Id::Type::state_var), input_selecter(this, "Input data series", Var_Id::Type::series) {
	
	Title("MobiView 2").MinimizeBox().Sizeable().Zoomable().Icon(MainIconImg::i4());
	
	SetDateFormat("%d-%02d-%02d");
	SetDateScan("ymd");
	
	plot_info_rect.Add(plot_info.HSizePos().VSizePos(0, 25));
	calib_label.SetText("Stat interval:");
	gof_option.SetLabel("Show GOF");
	gof_option.SetData(1); //TODO: Should this be in config?
	plot_info_rect.Add(calib_label.LeftPos(0).BottomPos(5));
	plot_info_rect.Add(calib_start.LeftPos(70, 70).BottomPos(0));
	plot_info_rect.Add(calib_end.LeftPos(145, 70).BottomPos(0));
	plot_info_rect.Add(gof_option.LeftPos(220).BottomPos(5));
	
	upper_horizontal.Horz();
	upper_horizontal.Add(par_group_selecter);
	upper_horizontal.Add(params);
	upper_horizontal.Add(plot_info_rect);
	upper_horizontal.Add(log_box);
	
	upper_horizontal.SetPos(1500, 0).SetPos(7000, 1).SetPos(8500, 2);

	lower_horizontal.Horz();
	lower_horizontal.Add(plotter);
	lower_horizontal.Add(result_selecter);
	lower_horizontal.Add(input_selecter);
	
	lower_horizontal.SetPos(7000, 0).SetPos(8500, 1);

	main_vertical.Vert(upper_horizontal, lower_horizontal);
	
	main_vertical.SetPos(3000, 0);

	Add(main_vertical);
	
	
	WhenClose = THISBACK(closing_checks);
	
	AddFrame(tool_bar);
	tool_bar.Set(THISBACK(sub_bar));
	
	
	log_box.SetEditable(false);
	plot_info.SetEditable(false);
	
	
	par_group_selecter.SetRoot(Null, String("Parameter groups"));
	par_group_selecter.SetNode(0, par_group_selecter.GetNode(0).CanSelect(false));
	par_group_selecter.HighlightCtrl(true);
	
	
	auto sensitivity_window_update = [this]() {
		sensitivity_window.update();
	};
	
	//TODO: This is not sufficient. It is not updated when selection changes within an individual row!
	// What we want is something like WhenLeftClick, but that
	// doesn't work either! May have to change to GridCtrl instead of ArrayCtrl for this to
	// work.
	params.parameter_view.WhenSel << sensitivity_window_update;
	
	par_group_selecter.WhenSel << [this](){ params.par_group_change(); };
	par_group_selecter.WhenSel << sensitivity_window_update;
	
	
	calib_start.WhenAction = THISBACK(plot_rebuild);
	calib_end.WhenAction   = THISBACK(plot_rebuild);
	gof_option.WhenAction  = THISBACK(plot_rebuild);
	
	
	//Load in some settings
	String settings_file = LoadFile(GetDataFile("settings.json"));
	Value  settings_json = ParseJSON(settings_file);
	
	//NOTE: We have to load in this info here already, otherwise it is lost when exiting the
	//application before loading in files.
	String previous_model     = settings_json["Model file"];
	String previous_data      = settings_json["Data file"];
	model_file                = previous_model.ToStd();
	data_file                 = previous_data.ToStd();
	
	const Value &window_dim = settings_json["Window dimensions"];
	if(window_dim.GetCount() == 2 && (int)window_dim[0] > 0 && (int)window_dim[1] > 0)
		SetRect(0, 0, (int)window_dim[0], (int)window_dim[1]);
	
	const Value &main_vert = settings_json["Main vertical"];
	if((int)main_vert > 0)
		main_vertical.SetPos((int)main_vert, 0);
	
	const Value &upper_horz = settings_json["Upper horizontal"];
	if(upper_horz.GetCount() == upper_horizontal.GetCount()) {
		for(int Idx = 0; Idx < upper_horz.GetCount(); ++Idx)
			upper_horizontal.SetPos((int)upper_horz[Idx], Idx);
	}
	
	const Value &lower_horz = settings_json["Lower horizontal"];
	if(lower_horz.GetCount() == lower_horizontal.GetCount()) {
		for(int Idx = 0; Idx < lower_horz.GetCount(); ++Idx)
			lower_horizontal.SetPos((int)lower_horz[Idx], Idx);
	}
	
	if((bool)settings_json["Maximize"]) Maximize();
	
	stat_settings.read_from_json(settings_json);
	
	plotter.main_plot.build_plot(); // NOTE: Just to make it initially set the message that no model is loaded
}

void MobiView2::log(String msg, bool error) {
	
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::stringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S : ");
	
	msg.Replace("&", "`&");
	String format_msg = "";
	format_msg << oss.str().data();
	format_msg << msg;

	format_msg = DeQtf(format_msg);

	format_msg.Replace("\n", "&");
	format_msg.Replace("\t", "-|");
	format_msg << "&&";
	
	if(error)
		format_msg = String("[@R ") + format_msg + "]";
	
	log_box.append(format_msg);
}

void MobiView2::log_warnings_and_errors() {
	std::string warnbuf = global_log_stream.str();
	if(warnbuf.size() > 0) {
		log(warnbuf.data());
		global_log_stream.str("");
	}
	
	std::string errbuf = global_error_stream.str();
	if(errbuf.size() > 0) {
		log(errbuf.data(), true);
		global_error_stream.str("");
	}
}

void MobiView2::sub_bar(Bar &bar) {
	bar.Add(IconImg::Open(), THISBACK(load)).Tip("Load model and data files").Key(K_CTRL_O);
	bar.Add(IconImg::ReLoad(), [this](){ reload(); } ).Tip("Reload the already loaded model and data files").Key(K_F5);
	bar.Add(IconImg::Save(), THISBACK(save_parameters)).Tip("Save parameters").Key(K_CTRL_S);
	bar.Add(IconImg::SaveAs(), THISBACK(save_parameters_as)).Tip("Save parameters as").Key(K_ALT_S);
	bar.Add(IconImg::Search(), THISBACK(open_search_window)).Tip("Search parameters").Key(K_CTRL_F);
	bar.Add(IconImg::ViewReaches(), THISBACK(open_view_chart)).Tip("Model chart and index distribution");
	bar.Separator();
	bar.Add(IconImg::Run(), THISBACK(run_model)).Tip("Run model").Key(K_F7);
	bar.Add(IconImg::ViewMorePlots(), THISBACK(open_additional_plots)).Tip("Open additional plot view");
	//bar.Add(IconImg::SaveCsv(), THISBACK(SaveToCsv)).Tip("Save results to .csv").Key(K_CTRL_R);
	bar.Separator();
	//bar.Gap(60);
	bar.Add(IconImg::SaveBaseline(), THISBACK(save_baseline)).Tip("Save baseline");
	bar.Add(IconImg::RevertBaseline(), THISBACK(revert_baseline)).Tip("Revert to baseline");
	bar.Add(IconImg::Perturb(), THISBACK(open_sensitivity_window)).Tip("Parameter perturbation (sensitivity)");
	bar.Add(IconImg::Optimize(), THISBACK(open_optimization_window)).Tip("Optimization and MCMC setup");
	bar.Separator();
	bar.Add(IconImg::StatSettings(), THISBACK(open_stat_settings)).Tip("Edit statistics settings");
	bar.Add(IconImg::BatchStructure(), THISBACK(open_structure_view)).Tip("View model structure debug information");
	bar.Add(IconImg::Info(), THISBACK(open_info_window)).Tip("View model information");
}

void MobiView2::delete_model() {
	if(app) delete app;
	app = nullptr;
	if(model) delete model;
	model = nullptr;
	if(data_set) delete data_set;
	data_set = nullptr;
	if(baseline) delete baseline;
	baseline = nullptr;
}

bool MobiView2::do_the_load() {
	//NOTE: If a model was previously loaded, we have to do cleanup to prepare for a new load.
	if(model_is_loaded())	{
		//TODO!
		delete_model();
		
		params.changed_since_last_save = false;
		clean_interface();
		
		calib_start.SetData(Null);
		calib_end.SetData(Null);
	}
	
	if(!config_is_loaded)
		load_config();
	if(!config_is_loaded)
		return false;
	
	bool success = true;
#if CATCH_ERRORS
	try {
#endif
		data_set = new Data_Set;
		data_set->read_from_file(data_file.c_str());
		
		Model_Options options;
		data_set->get_model_options(options);
		
		model = load_model(model_file.data(), &mobius_config, &options);
		app = new Model_Application(model);
		
		app->build_from_data_set(data_set);
		app->compile(true);
		
#if CATCH_ERRORS
	} catch(int) {
		success = false;
		delete_model();
	}
#endif
	log_warnings_and_errors();
	store_settings(false);

	if(!success)
		return false;
	
	build_interface();
	return true;
}

bool
select_group_recursive(TreeCtrl &tree, int node, Entity_Id group_id) {
	Upp::Ctrl *ctrl = ~tree.GetNode(node).ctrl;
	if(ctrl) {
		if(group_id == reinterpret_cast<Entity_Node *>(ctrl)->entity_id) {
			tree.SetFocus();
			tree.SetCursor(node);
			return true;
		}
	}
	for(int idx = 0; idx < tree.GetChildCount(node); ++idx) {
		if(select_group_recursive(tree, tree.GetChild(node, idx), group_id))
			return true;
	}
	return false;
}

bool
MobiView2::select_par_group(Entity_Id group_id) {
	return select_group_recursive(par_group_selecter, 0, group_id);
}

Entity_Id
MobiView2::get_selected_par_group() {
	Entity_Id par_group_id = invalid_entity_id;
	Vector<int> selected_groups = par_group_selecter.GetSel();
	if(!selected_groups.empty()) {
		Ctrl *ctrl = ~par_group_selecter.GetNode(selected_groups[0]).ctrl;
		if(ctrl)
			par_group_id = reinterpret_cast<Entity_Node *>(ctrl)->entity_id;
	}
	return par_group_id;
}

void MobiView2::reload(bool recompile_only) {
	if(!model_is_loaded()) {
		if(data_file.empty() || model_file.empty())
			log("Not able to reload when there is nothing loaded to begin with.", true);
		else
			log("Attempting to reload the model.");
			// This branch could happen if there was a previous attempt at loading which gave
			// an error. There is still a valid file name for the model and data file,
			// but the model is not loaded.
			// In this case the interface was already cleared, so there is no point in
			// restoring it like below unless we decide to cache all that information somewhere.
			do_the_load();
		return;
	}
	//TODO: decide what to do about changed parameters. (are they saved first?)
	
	log(Format("Reloading the model \"%s\"", app->model->model_name));
	
	if(!recompile_only && params.changed_since_last_save) {
		int close = PromptYesNo("Parameters have been edited since the last save. If you reload now, you will lose the changes. Continue anyway?");
		if(!close) return;
	}
	
	bool model_was_already_run = (app->data.results.time_steps > 0);
	
	// We serialize the selected setup by names since all the ids could have changed when we
	// recompile.
	
	std::string selected_group;
	
	String plot_setup_data;
	Vector<String> additional_setup_data;

#if CATCH_ERRORS
	try {   // Note this should really not give an error, but could if we messed up the serialization code.
#endif
		// Selected parameter group.
		auto par_group_id = get_selected_par_group();
		if(is_valid(par_group_id))
			selected_group = model->serialize(par_group_id);
		
		// TODO: Also the additional plot view.
		plot_setup_data = serialize_plot_setup(app, plotter.main_plot.setup);
		
		additional_setup_data = additional_plots.serialize_setups();
#if CATCH_ERRORS
	} catch(int) {
		log_warnings_and_errors();
	}
#endif
	
	bool resized = plotter.main_plot.was_auto_resized;
	
	bool success = true;
	if(!recompile_only) {
		success = do_the_load();
	} else {
		#if CATCH_ERRORS
		try {
		#endif
			clean_interface();
			app->save_to_data_set();
			
			if(baseline) delete baseline;
			baseline = nullptr;
			
			delete app;
			app = new Model_Application(model);
			app->build_from_data_set(data_set);
			app->compile(true);
			build_interface();
		#if CATCH_ERRORS
		} catch(int) {
			delete_model();
			success = false;
		}
		#endif
		log_warnings_and_errors();
		//store_settings(false);
	}
	
	if(!success) return;
	
	Plot_Setup setup = deserialize_plot_setup(app, plot_setup_data);
	std::vector<Plot_Setup> additional_setups;
	for(auto &data : additional_setup_data)
		additional_setups.push_back(deserialize_plot_setup(app, data));
	additional_plots.set_all(additional_setups);
	
	plotter.main_plot.was_auto_resized = resized; // A bit hacky, but should cause it to not re-size x axis if it is already set.
	plotter.set_plot_setup(setup);
	
	if(selected_group != "") {
		auto group_id = model->deserialize(selected_group, Reg_Type::par_group);
		if(is_valid(group_id))
			select_par_group(group_id);
	}
	
	log("The model was reloaded.");
	
	if(!recompile_only && model_was_already_run)
		run_model();
}

void MobiView2::load_config() {
	#if CATCH_ERRORS
		try {
	#endif
			mobius_config = ::load_config();
			config_is_loaded = true;
	#if CATCH_ERRORS
		} catch(int) {
			log_warnings_and_errors();
			return;
		}
	#endif
}

void MobiView2::load() {
	
	if(params.changed_since_last_save) {
		int close = PromptYesNo("Parameters have been edited since the last save. If you load a new dataset now, you will lose the changes. Continue anyway?");
		if(!close) return;
	}

	String settings_file = LoadFile(GetDataFile("settings.json"));
	Value  settings_json = ParseJSON(settings_file);
	
	String previous_model = settings_json["Model file"];
	String previous_data  = settings_json["Data file"];
	
	if(!config_is_loaded)
		load_config();
	if(!config_is_loaded)
		return;
		
	std::string &mobius_base_path = mobius_config.mobius_base_path;
	
	FileSel model_sel;

	model_sel.Type("Model files", "*.txt");     //TODO: Decide on what we want to call them!
		
	if(!previous_model.IsEmpty()) {
		if(FileExists(previous_model))
			model_sel.PreSelect(previous_model);
		else {
			auto folder = GetFileFolder(previous_model);
			if(DirectoryExists(folder))
				model_sel.ActiveDir(folder);
		}
	} else if(!mobius_base_path.empty())
		model_sel.ActiveDir(mobius_base_path.data());
	else
		model_sel.ActiveDir(GetCurrentDirectory());
	
	model_sel.ExecuteOpen();
	std::string new_model_file = model_sel.Get().ToStd();
	
	bool changed_model = new_model_file != previous_model.ToStd();
	
	if(new_model_file.empty()) {
		log("Received empty model file name.", true);
		return;
	}
	model_file = new_model_file;
	
	log(Format("Selecting model: %s", model_file.data()));

	FileSel data_sel;
	data_sel.Type("Input .dat files", "*.dat");

	if(!changed_model && !previous_data.IsEmpty() && FileExists(previous_data))
		data_sel.PreSelect(previous_data);
	else {
		auto previous_for_model = settings_json["Last data"][model_file.c_str()];
		if(!IsNull(previous_for_model) && FileExists(previous_for_model.ToString())) {
			data_sel.PreSelect(previous_for_model);
		} else
			data_sel.ActiveDir(GetFileFolder(model_file.data()));
	}
	data_sel.ExecuteOpen();
	std::string new_data_file = data_sel.Get().ToStd();
	
	if(new_data_file.empty()){
		log("Received empty data file name.", true);
		return;
	}
	data_file = new_data_file;
	
	bool changed_data_path = GetFileDirectory(data_file.data()) != GetFileDirectory(previous_data);

	log(Format("Selecting data file: %s", data_file.data()));

	bool success = do_the_load();
	if(success)
		log(Format("Loaded model \"%s\"", app->model->model_name));
}

void MobiView2::save_parameters() {
	if(!model_is_loaded() || data_file.size()==0) {
		log("Parameters can only be saved once a model and data file is loaded.", true);
		return;
	}
	
#if CATCH_ERRORS
	try {
#endif
		app->save_to_data_set();
		data_set->write_to_file(data_file);
		
		log(Format("Parameters saved to %s", data_file.data()));
		params.changed_since_last_save = false;
#if CATCH_ERRORS
	} catch(int) {
		log("Data file saving may have been unsuccessful.", true);
	}
#endif
	log_warnings_and_errors();
}

void MobiView2::save_parameters_as() {
	if(!model_is_loaded()) {
		log("Parameters can only be saved once a model and data files is loaded.", true);
		return;
	}
	FileSel sel;
	sel.Type("Data .dat files", "*.dat");
	if(data_file.size() > 0) {
		String existing_file = data_file.data();
		sel.PreSelect(existing_file);
	}
	sel.ExecuteSaveAs("Save data file as");
	
	std::string new_file = sel.Get().ToStd();
	if(new_file.size() == 0) return;
	
#if CATCH_ERRORS
	try {
#endif
		app->save_to_data_set();
		data_set->write_to_file(new_file);
		
		log(Format("Parameters saved to %s", new_file.data()));
		params.changed_since_last_save = false;
		data_file = new_file;
		
		store_settings(); // NOTE: so that it records the new file as the data file.
#if CATCH_ERRORS
	} catch(int) {
		log("Data file saving may have been unsuccessful.", true);
	}
#endif
	log_warnings_and_errors();
}

void MobiView2::plot_rebuild() {
	if(!app) return;
	// If the calibration interval is not set, set it based on the last model run.
	s64 time_steps = app->data.results.time_steps;
	if(time_steps > 0) {
		Time start_time = calib_start.GetData();
		Time end_time   = calib_end.GetData();
		if (IsNull(start_time))
			calib_start.SetData(convert_time(app->data.results.start_date));
		if (IsNull(end_time)) {
			Date_Time end_date = advance(app->data.results.start_date, app->time_step_size, time_steps-1);
			calib_end.SetData(convert_time(end_date));
		}
	}
	
	plotter.re_plot(true);
	if(additional_plots.IsOpen())
		additional_plots.build_all(true);
	baseline_was_just_saved = false;
}

void run_callback(void *callback_data, double percent) {
	auto mv = (MobiView2 *)callback_data;
	mv->log_box.set_progress(percent);
	mv->ProcessEvents();
}

void MobiView2::run_model() {
	if(!model_is_loaded()) {
		log("The model can only be run if it is loaded along with a data file.", true);
		return;
	}
	
#if CATCH_ERRORS
	try {
#endif
		
		// This is to avoid it updating the plot during long model runs.
		// Although it was a bit cool to see the progress, it is bug prone since
		// it is working with arrays that are potentially not of expected size.
		plotter.main_plot.clean(false);

		s64 timeout = stat_settings.ms_timeout.GetData();
		bool check_for_nan = stat_settings.check_for_nan.GetData();
		
		log(Format("Starting model run%s.", check_for_nan ? " (with checks for non-finite values turned on)" : ""));
		log_box.add_progress_bar();
		
		ProcessEvents();
		Timer run_timer;
		bool finished = ::run_model(app, timeout, check_for_nan, run_callback, this);
		double ms = run_timer.get_milliseconds();
		log_box.finish_progress_bar();
		
		if(finished) {
			if(ms > 5000) {
				log(Format("Model run finished.\nDuration: %g s.", ms/1000));
			} else {
				log(Format("Model run finished.\nDuration: %g ms.", ms));
			}
			if(mobius_developer_mode) {
				s64 kb = ((s64)app->data.results.alloc_size() / (1024*1024));
				s64 mb = kb/1024;
				
				if(mb >= 5)
					log(Format("Result allocation size was %dMB", mb));
				else
					log(Format("Result allocation size was %dkB", kb));
			}
		} else
			fatal_error("Model run failed to finish.");
#if CATCH_ERRORS
	} catch(int) {
	}
#endif
	log_warnings_and_errors();
	
	result_selecter.Enable();
	
	plot_rebuild();
	
	// TODO: This will probably not be needed any more:
	//params.refresh(true); //NOTE: In case there are computed parameters that are displayed, we need to refresh their values in the view
}

void MobiView2::store_settings(bool store_favorites) {
	String settings_file = LoadFile(GetDataFile("settings.json"));
	Value existing_settings = ParseJSON(settings_file);
	
	//ValueMap Eq = ExistingSettings["Favorite equations"];
	
	Json settings_json;
	
	settings_json
		("Model file", String(model_file))
		("Data file", String(data_file));
	
	JsonArray window_dim;
	window_dim << GetSize().cx << GetSize().cy;
	settings_json("Window dimensions", window_dim);
	
	settings_json("Main vertical splitter", main_vertical.GetPos(0));
	
	JsonArray upper_horz;
	for(int Idx = 0; Idx < upper_horizontal.GetCount(); ++Idx)
		upper_horz << upper_horizontal.GetPos(Idx);
	settings_json("Upper horizontal", upper_horz);
	
	JsonArray lower_horz;
	for(int Idx = 0; Idx < lower_horizontal.GetCount(); ++Idx)
		lower_horz << lower_horizontal.GetPos(Idx);
	settings_json("Lower horizontal", lower_horz);
	
	settings_json("Maximize", IsMaximized());
	
	stat_settings.write_to_json(settings_json);
	
	JsonArray plot_view_dim;
	plot_view_dim << additional_plots.GetSize().cx << additional_plots.GetSize().cy;
	settings_json("Plot view dimensions", plot_view_dim);
	
	// TODO: Store resizes of other windows here too.
	
	// Just write out the last selected data files again
	Json file_map;
	
	// Hmm, should we index this on the file name part of the model only, not the entire path?
	bool found_current = false;
	ValueMap file_map_before = existing_settings["Last data"];
	if(!IsNull(file_map_before)) {
		for(auto &k : file_map_before.GetKeys()) {
			auto modname = k.ToString();
			if(modname == model_file.data()) {
				found_current = true;
				file_map(modname, data_file.data());
			} else
				file_map(modname, file_map_before[k]);
		}
	}
	if(!found_current)
		file_map(model_file.data(), data_file.data());
	settings_json("Last data", file_map);
	
	SaveFile("settings.json", settings_json.ToString());
}

void MobiView2::clean_interface() {
	par_group_selecter.Clear();
	par_group_nodes.Clear();
	
	result_selecter.clean();
	input_selecter.clean();
	plot_info.Clear();
	
	search_window.clean();
	params.clean();
	plotter.clean();
	additional_plots.clean();
	optimization_window.clean();
	
	baseline_was_just_saved = false;
}

void MobiView2::build_interface() {
	if(!model_is_loaded()) {
		log("Tried to build the interface without a model having been loaded", true);
		return;
	}
	
	#if CATCH_ERRORS
	try {
	#endif
		par_group_selecter.Set(0, model->model_name.data());
		par_group_selecter.SetNode(0, par_group_selecter.GetNode(0).CanSelect(false)); //Have to reset this every time one changes the name of the node apparently.
		
		// Hmm, this is a bit cumbersome. See similar note in model_application.cpp
		for(int idx = -1; idx < model->modules.count(); ++idx) {
			
			int id = 0; // The node id of the module or model.
			
			Entity_Id module_id = idx >= 0 ? Entity_Id { Reg_Type::module, (s16)idx } : invalid_entity_id;
				
			// Don't display modules that don't have par_groups in the par_group_selecter
			auto iter = model->get_scope(module_id)->by_type(Reg_Type::par_group);
			if(iter.size() == 0)
				continue;
			
			if(is_valid(module_id)) {
				auto mod = model->modules[module_id];
				auto temp = model->module_templates[mod->template_id];
				
				String name = Format("%s v. %d.%d.%d", mod->full_name.data(), temp->version.major, temp->version.minor, temp->version.revision);
				id = par_group_selecter.Add(0, Null, name, false);
				par_group_selecter.SetNode(id, par_group_selecter.GetNode(id).CanSelect(false));
			}
			
			for(auto group_id : iter) {
				auto par_group = model->par_groups[group_id];
				if(par_group->decl_type == Decl_Type::option_group) continue;
				par_group_nodes.Create(group_id, par_group->name.data());
				par_group_selecter.Add(id, Null, par_group_nodes.Top(), false);
			}
		}
		par_group_selecter.OpenDeep(0, true);
		
		result_selecter.build(app);
		input_selecter.build(app);
	
		params.build_index_set_selecters(app);
		plotter.build_index_set_selecters(app);
		
		plotter.build_time_intervals_ctrl();
		
		plotter.plot_change();
		
		if(model_chart_window.IsOpen())
			model_chart_window.rebuild();
		if(structure_window.IsOpen())
			structure_window.refresh_text();
		if(info_window.IsOpen())
			info_window.refresh_text();
	#if CATCH_ERRORS
	} catch(int) {
	}
	#endif
	log_warnings_and_errors();
}

void MobiView2::closing_checks() {
	int close = 1;
	if(params.changed_since_last_save)
		close = PromptYesNo("Parameters have been edited since the last save. If you exit now you will loose any changes. Do you still want to exit MobiView2?");
	if(!close) return;
	
	store_settings();
	Close();
	
	// NOTE: necessary to cleanly free the memory used by the LLVM jit.
	delete_model();
}

void MobiView2::save_baseline() {
	if(!model_is_loaded()) {
		log("You can only save a baseline after the model has been run once", true);
		return;
	}
	//TODO!
	if(baseline) delete baseline;
	baseline = app->data.copy();
	baseline_was_just_saved = true;
	log("Saved baseline.");
	plot_rebuild();
}

void MobiView2::revert_baseline() {
	//TODO!
	if(!baseline) {
		log("Can not revert to the baseline since no baseline was saved.", true);
		return;
	}
	app->data.parameters.copy_from(&baseline->parameters);
	app->data.results.copy_from(&baseline->results);
	plot_rebuild();
	params.refresh_parameter_view(true);
	log("Reverted to baseline.");
}

void MobiView2::open_stat_settings() {
	if(stat_settings.IsOpen()) return;
	stat_settings.load_interface();
	stat_settings.Open();
}

void MobiView2::open_search_window() {
	if(search_window.IsOpen()) return;
	search_window.Open();
	search_window.search_field.SetFocus();
}

void MobiView2::open_sensitivity_window() {
	if(sensitivity_window.IsOpen()) return;
	sensitivity_window.Open();
}

void MobiView2::open_info_window() {
	if(info_window.IsOpen()) return;
	info_window.refresh_text();
	info_window.Open();
}

void MobiView2::open_additional_plots() {
	if(additional_plots.IsOpen()) return;
	additional_plots.Open();
	additional_plots.build_all();
}

void MobiView2::open_optimization_window() {
	if(optimization_window.IsOpen()) return;
	optimization_window.Open();
}

void MobiView2::open_structure_view() {
	if(structure_window.IsOpen()) return;
	structure_window.refresh_text();
	structure_window.Open();
}

void MobiView2::open_view_chart() {
	if(model_chart_window.IsOpen()) return;
	model_chart_window.rebuild();
	model_chart_window.Open();
}

GUI_APP_MAIN {
	StdLogSetup(LOG_FILE, "MobiView2.log");
	MobiView2().Run();
}

#undef CATCH_ERRORS