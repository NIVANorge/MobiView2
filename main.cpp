#include "MobiView2.h"

#define IMAGECLASS IconImg
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

#define IMAGECLASS MainIconImg
#define IMAGEFILE <MobiView2/MobiView2.iml>
#include <Draw/iml.h>

#include <iomanip>

std::stringstream global_error_stream;
std::stringstream global_warning_stream;

using namespace Upp;

MobiView2::MobiView2() :
	params(this), plotter(this), stat_settings(this), search_window(this),
	sensitivity_window(this), info_window(this), additional_plots(this),
	optimization_window(this), mcmc_window(this), structure_window(this),
	result_selecter(this, "Result time series", Var_Id::Type::state_var), input_selecter(this, "Input time series", Var_Id::Type::series) {
	
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
	
	par_group_selecter.WhenSel << [this](){ params.refresh(false); };
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
	/*

	Value IdxEditWindowDim = settings_json["Index set editor window dimensions"];
	if(IdxEditWindowDim.GetCount() == 2 && (int)IdxEditWindowDim[0] > 0 && (int)IdxEditWindowDim[1] > 0)
		ChangeIndexes.SetRect(0, 0, (int)IdxEditWindowDim[0], (int)IdxEditWindowDim[1]);
	
	Value AdditionalPlotViewDim = settings_json["Additional plot view dimensions"];
	if(AdditionalPlotViewDim.GetCount() == 2 && (int)AdditionalPlotViewDim[0] > 0 && (int)AdditionalPlotViewDim[1] > 0)
		OtherPlots.SetRect(0, 0, (int)AdditionalPlotViewDim[0], (int)AdditionalPlotViewDim[1]);
	*/
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
	format_msg << "&&";
	format_msg.Replace("[", "`[");
	format_msg.Replace("]", "`]");
	format_msg.Replace("_", "`_");
	format_msg.Replace("<", "`<");
	format_msg.Replace(">", "`>");
	format_msg.Replace("-", "`-");
	format_msg.Replace("/", "\1/\1");
	format_msg.Replace("\\", "\1\\\1");
	format_msg.Replace("\n", "&");
	format_msg.Replace("\t", "-|");
	
	if(error)
		format_msg = String("[@R ") + format_msg + "]";
	
	log_box.append(format_msg);
	log_box.ScrollEnd();
}

void MobiView2::log_warnings_and_errors() {
	std::string warnbuf = global_warning_stream.str();
	if(warnbuf.size() > 0) {
		log(warnbuf.data());
		global_warning_stream.str("");
	}
	
	std::string errbuf = global_error_stream.str();
	if(errbuf.size() > 0) {
		log(errbuf.data(), true);
		global_error_stream.str("");
	}
}

void MobiView2::sub_bar(Bar &bar) {
	bar.Add(IconImg::Open(), THISBACK(load)).Tip("Load model and data files").Key(K_CTRL_O);
	bar.Add(IconImg::ReLoad(), [this](){ reload(); } ).Tip("Reload the already loaded model and data files.").Key(K_F5);
	bar.Add(IconImg::Save(), THISBACK(save_parameters)).Tip("Save parameters").Key(K_CTRL_S);
	bar.Add(IconImg::SaveAs(), THISBACK(save_parameters_as)).Tip("Save parameters as").Key(K_ALT_S);
	bar.Add(IconImg::Search(), THISBACK(open_search_window)).Tip("Search parameters").Key(K_CTRL_F);
	//bar.Add(IconImg::ViewReaches(), THISBACK(OpenChangeIndexes)).Tip("Edit indexes");
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

#ifdef _DEBUG
	#define CATCH_ERRORS 0
#else
	#define CATCH_ERRORS 1
#endif

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
	
	bool success = true;
#if CATCH_ERRORS
	try {
#endif
		model = load_model(model_file.data());
		app = new Model_Application(model);
			
		data_set = new Data_Set;
		data_set->read_from_file(data_file.data());
		
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

#undef CATCH_ERRORS

void MobiView2::reload(bool recompile_only) {
	if(!model_is_loaded()) {
		if(data_file.empty() || model_file.empty())
			log("Not able to reload when there is nothing loaded to begin with.", true);
		else
			// This branch could happen if there was a previous attempt at loading which gave
			// an error. There is still a valid file name for the model and data file,
			// but the model is not loaded.
			// In this case the interface was already cleared, so there is no point in
			// restoring it like below unless we decide to cache all that information somewhere.
			do_the_load();
		return;
	}
	//TODO: decide what to do about changed parameters.
	
	//NOTE: we have to get out the names of selected series and indexes, since the Var_Ids may have changed after the reload.
	// TODO: this should probably be factored out to be reused in serialization..
	Plot_Setup setup = plotter.main_plot.setup;
	std::vector<std::string> sel_results;
	std::vector<std::string> sel_inputs;
	std::vector<std::string> sel_additional;
	std::vector<std::vector<std::string>> sel_indexes(MAX_INDEX_SETS);
	for(Var_Id var_id : setup.selected_results)
		sel_results.push_back(app->state_vars.serialize(var_id));
	for(Var_Id var_id : setup.selected_series) {
		if(var_id.type == Var_Id::Type::series)
			sel_inputs.push_back(app->series.serialize(var_id));
		else
			sel_additional.push_back(app->additional_series.serialize(var_id));
	}
	//TODO: the index sets themselves could change (or change order). So we have to store their
	//names and remap the order! Also rebuild "index set is active"
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		for(Index_T index : setup.selected_indexes[idx]) {
			if(!is_valid(index)) continue;
			// TODO: this is a bit unsafe. Maybe make api for it in model_application.h
			sel_indexes[idx].push_back(app->get_index_name(index));
		}
	}
	
	// Selected parameter group.
	//TODO: not sure if scoping it to the module will be necessary eventually.
	String selected_group;
	Vector<int> selected_groups = par_group_selecter.GetSel();
	if(!selected_groups.empty()) {
		Ctrl *ctrl = ~par_group_selecter.GetNode(selected_groups[0]).ctrl;
		if(ctrl) {
			Entity_Id par_group_id = reinterpret_cast<Entity_Node *>(ctrl)->entity_id;
			auto par_group = model->par_groups[par_group_id];
			selected_group = par_group->name;
		}
	}
	
	bool resized = plotter.main_plot.was_auto_resized;
	
	bool success = true;
	if(!recompile_only) {
		success = do_the_load();
	} else {
		try {
			app->save_to_data_set();
			delete app;
			app = new Model_Application(model);
			app->build_from_data_set(data_set);
			app->compile(true);
		} catch(int) {
			delete_model();
			success = false;
		}
		log_warnings_and_errors();
		//store_settings(false);
	}
	
	if(!success) return;
	
	// TODO: it is not ideal for this functionality that series names may not be unique. Fix
	// this in Mobius 2?
	setup.selected_results.clear();
	setup.selected_series.clear();
	for(auto &name : sel_results) {
		Var_Id var_id = app->state_vars.deserialize(name);
		if(is_valid(var_id))
			setup.selected_results.push_back(var_id);
	}
	for(auto &name : sel_inputs) {
		Var_Id var_id = app->series.deserialize(name);
		if(is_valid(var_id))
			setup.selected_series.push_back(var_id);
	}
	for(auto &name : sel_additional) {
		Var_Id var_id = app->additional_series.deserialize(name);
		if(is_valid(var_id))
			setup.selected_series.push_back(var_id);
	}
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		setup.selected_indexes[idx].clear();
		for(auto &name : sel_indexes[idx]) {
			Index_T index = app->get_index({Reg_Type::index_set, idx}, name);
			if(is_valid(index))
				setup.selected_indexes[idx].push_back(index);
		}
	}
	
	plotter.main_plot.was_auto_resized = resized; // A bit hacky, but should cause it to not re-size x axis if it is already set.
	plotter.set_plot_setup(setup);
	
	// TODO: doesn't work for top level parameter groups. Factor out stuff from search window
	// instead.
	bool breakout = false;
	if(selected_group != "") {
		for(int node_id = 0; node_id < par_group_selecter.GetChildCount(0); ++node_id) {
			for(int node_id2 = 0; node_id2 < par_group_selecter.GetChildCount(node_id); ++node_id2) {
				Ctrl *ctrl = ~par_group_selecter.GetNode(node_id2).ctrl;
				if(ctrl) {
					Entity_Id par_group_id = reinterpret_cast<Entity_Node *>(ctrl)->entity_id;
					auto par_group = model->par_groups[par_group_id];
					if(selected_group == String(par_group->name)) {
						par_group_selecter.SetFocus();
						par_group_selecter.SetCursor(node_id2);
						breakout = true;
						break;
					}
				}
			}
			if(breakout) break;
		}
	}
	
	if(!recompile_only)
		run_model();
}

void MobiView2::load() {
	
	if(params.changed_since_last_save) {
		int close = PromptYesNo("Parameters have been edited since the last save. If you load a new dataset now, you will lose them. Continue anyway?");
		if(!close) return;
	}

	String settings_file = LoadFile(GetDataFile("settings.json"));
	Value  settings_json = ParseJSON(settings_file);
	
	String previous_model = settings_json["Model file"];
	String previous_data  = settings_json["Data file"];
	
	FileSel model_sel;

	model_sel.Type("Model files", "*.txt");     //TODO: Decide on what we want to call them1

	if(!previous_model.IsEmpty()) {
		if(FileExists(previous_model))
			model_sel.PreSelect(previous_model);
		else {
			auto folder = GetFileFolder(previous_model);
			if(DirectoryExists(folder))
				model_sel.ActiveDir(folder);
		}
	}
	
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
#ifdef PLATFORM_WIN32
	data_sel.Type("Input .dat or spreadsheet files", "*.dat *.xls *.xlsx");
#else
	data_sel.Type("Input .dat files", "*.dat");
#endif

	if(!changed_model && !previous_data.IsEmpty() && FileExists(previous_data))
		data_sel.PreSelect(previous_data);
	else
		data_sel.ActiveDir(GetFileFolder(model_file.data()));
	data_sel.ExecuteOpen();
	std::string new_data_file = data_sel.Get().ToStd();
	
	if(new_data_file.empty()){
		log("Received empty data file name.", true);
		return;
	}
	data_file = new_data_file;
	
	bool changed_data_path = GetFileDirectory(data_file.data()) != GetFileDirectory(previous_data);

	log(Format("Selecting data file: %s", data_file.data()));

	do_the_load();
}

void MobiView2::save_parameters() {
	if(!model_is_loaded() || data_file.size()==0) {
		log("Parameters can only be saved once a model and data file is loaded.", true);
		return;
	}
	
	try {
		app->save_to_data_set();
		data_set->write_to_file(data_file);
		
		log(Format("Parameters saved to %s", data_file.data()));
		params.changed_since_last_save = false;
	} catch(int) {
		log("Data file saving may have been unsuccessful.", true);
	}
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
	
	try {
		app->save_to_data_set();
		data_set->write_to_file(new_file);
		
		log(Format("Parameters saved to %s", new_file.data()));
		params.changed_since_last_save = false;
		data_file = new_file;
		
		store_settings(); // NOTE: so that it records the new file as the data file.
	} catch(int) {
		log("Data file saving may have been unsuccessful.", true);
	}
	log_warnings_and_errors();
}

void MobiView2::plot_rebuild() {
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

void MobiView2::run_model() {
	if(!model_is_loaded()) {
		log("The model can only be run if it is loaded along with a data file.", true);
		return;
	}
	
	log("Starting model run.");
	ProcessEvents();
	try {
		Timer run_timer;
		::run_model(app, -1);
		double ms = run_timer.get_milliseconds();
		
		log(Format("Model run finished.\nDuration: %g ms.", ms ));
	} catch(int) {
		
	}
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
	
	/*
	Json Favorites;
	
	String DllFileName = GetFileName(DllFile.data());
	
	for(const auto &K : Eq.GetKeys())
	{
		if(K != DllFileName || !OverwriteFavorites || !ModelDll.IsLoaded())
			Favorites(K.ToString(), Eq[K]);
	}
	
	if(OverwriteFavorites && ModelDll.IsLoaded()) // If the dll file is not actually loaded, the favorites are not stored in the EquationSelecter, just keep what was there originally instead
	{
		JsonArray FavForCurrent;
		for(int Row = 0; Row < EquationSelecter.GetCount(); ++Row)
		{
			if(EquationSelecter.Get(Row, 1)) //I.e. if it was favorited
				FavForCurrent << EquationSelecter.Get(Row, 0);
		}
		Favorites(DllFileName, FavForCurrent);
	}
	
	SettingsJson("Favorite equations", Favorites);
	*/
	
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
	/*
	JsonArray IdxEditWindowDim;
	IdxEditWindowDim << ChangeIndexes.GetSize().cx << ChangeIndexes.GetSize().cy;
	SettingsJson("Index set editor window dimensions", IdxEditWindowDim);
	*/
	
	JsonArray plot_view_dim;
	plot_view_dim << additional_plots.GetSize().cx << additional_plots.GetSize().cy;
	settings_json("Plot view dimensions", plot_view_dim);
	
	// TODO: Store resizes of other windows here too.
	
	SaveFile("settings.json", settings_json.ToString());
}

void MobiView2::clean_interface() {
	par_group_selecter.Clear();
	par_group_nodes.Clear();
	
	result_selecter.clean();
	input_selecter.clean();
	plot_info.Clear();
	
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
	
	par_group_selecter.Set(0, model->model_name.data());
	par_group_selecter.SetNode(0, par_group_selecter.GetNode(0).CanSelect(false)); //Have to reset this every time one changes the name of the node apparently.
	
	// Hmm, this is a bit cumbersome. See similar note in model_application.cpp
	for(int idx = -1; idx < model->modules.count(); ++idx) {
		Entity_Id module_id = invalid_entity_id;
		int id = 0;
		
		if(idx >= 0) {
			module_id = { Reg_Type::module, idx };
			
			auto mod = model->modules[module_id];
			if(!mod->has_been_processed) continue;
			
			String name = Format("%s v. %d.%d.%d", mod->name.data(), mod->version.major, mod->version.minor, mod->version.revision);
			id = par_group_selecter.Add(0, Null, name, false);
			par_group_selecter.SetNode(id, par_group_selecter.GetNode(id).CanSelect(false));
		}
		for(auto group_id : model->by_scope<Reg_Type::par_group>(module_id)) {
			auto par_group = model->par_groups[group_id];
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
}

void MobiView2::closing_checks() {
	int close = 1;
	if(params.changed_since_last_save)
		close = PromptYesNo("Parameters have been edited since the last save. If you exit now you will loose any changes. Do you still want to exit MobiView2?");
	if(!close) return;
	
	store_settings();
	
	// NOTE: necessary to cleanly free the memory used by the LLVM jit.
	delete_model();
	
	Close();
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
	params.refresh(true);
	log("Reverted to baseline.");
}

void MobiView2::open_stat_settings() {
	if(!stat_settings.IsOpen()) {
		stat_settings.load_interface();
		stat_settings.Open();
	}
}

void MobiView2::open_search_window() {
	if(!search_window.IsOpen())
		search_window.Open();
}

void MobiView2::open_sensitivity_window() {
	if(!sensitivity_window.IsOpen())
		sensitivity_window.Open();
}

void MobiView2::open_info_window() {
	if(!info_window.IsOpen()) {
		info_window.refresh_text();
		info_window.Open();
	}
}

void MobiView2::open_additional_plots() {
	if(!additional_plots.IsOpen()) {
		additional_plots.Open();
		additional_plots.build_all();
	}
}

void MobiView2::open_optimization_window() {
	if(!optimization_window.IsOpen())
		optimization_window.Open();
}

void MobiView2::open_structure_view() {
	if(!structure_window.IsOpen()) {
		structure_window.refresh_text();
		structure_window.Open();
	}
}

GUI_APP_MAIN {
	StdLogSetup(LOG_FILE, "MobiView2.log");
	MobiView2().Run();
}
