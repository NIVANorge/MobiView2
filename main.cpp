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

MobiView2::MobiView2() : params(this), plotter(this) {
	
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
	
	result_selecter_rect.Add(result_selecter.HSizePos().VSizePos(0, 30));
	result_selecter_rect.Add(show_favorites.SetLabel("Show favorites only").HSizePos(5).BottomPos(5));
	
	lower_horizontal.Horz();
	lower_horizontal.Add(plotter);
	lower_horizontal.Add(result_selecter_rect);
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
	
	
	//CurrentSelectedParameter.Valid = false;
	
	/*
	auto SensitivityWindowUpdate = [this]()
	{
		//NOTE: This HAS to be done before the update of the sensitivity window.
		//It is also used by the Optimization window. We have to do it here since
		//the parameters can be out of focus when in the other window!
		this->CurrentSelectedParameter = this->GetSelectedParameter();
		
		SensitivityWindow.Update();
	};
	*/
	
	//TODO: This is not sufficient. It is not updated when selection changes within an individual row!
	// What we want is something like WhenLeftClick, but that
	// doesn't work either! Maybe we have to set one on each individual control?
	//Params.ParameterView.WhenSel << SensitivityWindowUpdate;
	
	par_group_selecter.WhenSel << [this](){ params.refresh(false); };
	//ParameterGroupSelecter.WhenSel << SensitivityWindowUpdate;
	
	
	
	result_selecter.Disable();
	result_selecter.WhenSel << [this]() { plotter.plot_change(); };
	result_selecter.MultiSelect();
	result_selecter.SetRoot(Null, String("Results"));
	result_selecter.HighlightCtrl(true);
	result_selecter.SetNode(0, result_selecter.GetNode(0).CanSelect(false));
	
	input_selecter.WhenSel << [this]() { plotter.plot_change(); };
	input_selecter.MultiSelect();
	input_selecter.SetRoot(Null, String("Input time series"));
	input_selecter.HighlightCtrl(true);
	input_selecter.SetNode(0, input_selecter.GetNode(0).CanSelect(false));
	
	//ShowFavorites.WhenAction = THISBACK(UpdateEquationSelecter);
	
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
	
	Value window_dim = settings_json["Window dimensions"];
	if(window_dim.GetCount() == 2 && (int)window_dim[0] > 0 && (int)window_dim[1] > 0)
		SetRect(0, 0, (int)window_dim[0], (int)window_dim[1]);
	
	Value main_vert = settings_json["Main vertical"];
	if((int)main_vert > 0)
		main_vertical.SetPos((int)main_vert, 0);
	
	Value upper_horz = settings_json["Upper horizontal"];
	if(upper_horz.GetCount() == upper_horizontal.GetCount()) {
		for(int Idx = 0; Idx < upper_horz.GetCount(); ++Idx)
			upper_horizontal.SetPos((int)upper_horz[Idx], Idx);
	}
	
	Value lower_horz = settings_json["Lower horizontal"];
	if(lower_horz.GetCount() == lower_horizontal.GetCount()) {
		for(int Idx = 0; Idx < lower_horz.GetCount(); ++Idx)
			lower_horizontal.SetPos((int)lower_horz[Idx], Idx);
	}
	
	if((bool)settings_json["Maximize"]) Maximize();
	
	/*
	ValueMap StatsJson = settings_json["Statistics"];
	#define SET_SETTING(Handle, Name, Type) \
		Value Handle##Json = StatsJson[#Handle]; \
		if(!IsNull(Handle##Json)) \
		{ \
			StatSettings.Display##Handle = (int)Handle##Json; \
		}
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type)
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	ValueArray Percentiles = StatsJson["Percentiles"];
	if(!IsNull(Percentiles) && Percentiles.GetCount() >= 1)
	{
		StatSettings.Percentiles.clear();
		for(Value Perc : Percentiles)
		{
			double Val = Perc;  //TODO: Should check that it is a valid value between 0 and 100. Would only be a problem if somebody edited it by hand though
			StatSettings.Percentiles.push_back(Val);
		}
	}
	
	Value PrecisionJson = StatsJson["Precision"];
	if(!IsNull(PrecisionJson))
		StatSettings.Precision = (int)PrecisionJson;
	Value BFIJson = StatsJson["BFIParam"];
	if(!IsNull(BFIJson))
		StatSettings.EckhardtFilterParam = (double)BFIJson;
	Value ShowInitial = StatsJson["ShowInitial"];
	if(!IsNull(ShowInitial))
		StatSettings.ShowInitialValue = (bool)ShowInitial;

	Value IdxEditWindowDim = settings_json["Index set editor window dimensions"];
	if(IdxEditWindowDim.GetCount() == 2 && (int)IdxEditWindowDim[0] > 0 && (int)IdxEditWindowDim[1] > 0)
		ChangeIndexes.SetRect(0, 0, (int)IdxEditWindowDim[0], (int)IdxEditWindowDim[1]);
	
	Value AdditionalPlotViewDim = settings_json["Additional plot view dimensions"];
	if(AdditionalPlotViewDim.GetCount() == 2 && (int)AdditionalPlotViewDim[0] > 0 && (int)AdditionalPlotViewDim[1] > 0)
		OtherPlots.SetRect(0, 0, (int)AdditionalPlotViewDim[0], (int)AdditionalPlotViewDim[1]);
	*/
	
	// NOTE: Just to make it initially set the message that no model is loaded
	
	/*
	Plotter.MainPlot.BuildPlot(this, nullptr, true, PlotInfo, true);
	
	Search.ParentWindow = this;
	StructureView.ParentWindow = this;
	ChangeIndexes.ParentWindow = this;
	ChangeIndexes.Branches.ParentWindow = this;
	EditStatSettings.ParentWindow = this;
	OtherPlots.ParentWindow = this;
	ModelInfo.ParentWindow = this;
	SensitivityWindow.ParentWindow = this;
	OptimizationWin.ParentWindow = this;
	MCMCResultWin.ParentWindow = this;
	*/
}

void MobiView2::log(String msg, bool error) {
	
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::stringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S : ");
	
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
	bar.Add(IconImg::Open(), THISBACK(load)).Tip("Load model, parameter and input files").Key(K_CTRL_O);
	bar.Add(IconImg::Save(), THISBACK(save_parameters)).Tip("Save parameters").Key(K_CTRL_S);
	bar.Add(IconImg::SaveAs(), THISBACK(save_parameters_as)).Tip("Save parameters as").Key(K_ALT_S);
	//bar.Add(IconImg::Search(), THISBACK(OpenSearch)).Tip("Search parameters").Key(K_CTRL_F);
	//bar.Add(IconImg::ViewReaches(), THISBACK(OpenChangeIndexes)).Tip("Edit indexes");
	bar.Separator();
	bar.Add(IconImg::Run(), THISBACK(run_model)).Tip("Run model").Key(K_F7);
	//bar.Add(IconImg::ViewMorePlots(), THISBACK(OpenAdditionalPlotView)).Tip("Open additional plot view");
	//bar.Add(IconImg::SaveCsv(), THISBACK(SaveToCsv)).Tip("Save results to .csv").Key(K_CTRL_R);
	bar.Separator();
	//bar.Gap(60);
	//bar.Add(IconImg::SaveBaseline(), THISBACK(SaveBaseline)).Tip("Save baseline");
	//bar.Add(IconImg::RevertBaseline(), THISBACK(RevertBaseline)).Tip("Revert to baseline");
	//bar.Add(IconImg::Perturb(), THISBACK(OpenSensitivityView)).Tip("Sensitivity perturbation");
	//bar.Add(IconImg::Optimize(), THISBACK(OpenOptimizationView)).Tip("Optimization and MCMC setup");
	//bar.Separator();
	//bar.Add(IconImg::StatSettings(), THISBACK(OpenStatSettings)).Tip("Edit statistics settings");
	//bar.Add(IconImg::BatchStructure(), THISBACK(OpenStructureView)).Tip("View model equation batch structure");
	//bar.Add(IconImg::Info(), THISBACK(OpenModelInfoView)).Tip("View model information");
}

void MobiView2::load() {
	
	if(params.changed_since_last_save) {
		int close = PromptYesNo("Parameters have been edited since the last save. If you load a new dataset now, you will lose them. Continue anyway?");
		if(!close) return;
	}
	
	//NOTE: If a model was previously loaded, we have to do cleanup to prepare for a new load.
	if(app)	{
		//TODO!
		
		//if(BaselineDataSet)
		//{
		//	ModelDll.DeleteDataSet(BaselineDataSet);
		//	BaselineDataSet = nullptr;
		//}
		
		//if(DataSet)
		//{
		//	ModelDll.DeleteModelAndDataSet(DataSet);
		//	DataSet = nullptr;
		//}
		
		params.changed_since_last_save = false;
		clean_interface();
		
		calib_start.SetData(Null);
		calib_end.SetData(Null);
	}
	
	String settings_file = LoadFile(GetDataFile("settings.json"));
	Value  settings_json = ParseJSON(settings_file);
	
	String previous_model = settings_json["Model file"];
	String previous_data  = settings_json["Data file"];
	
	FileSel model_sel;

	model_sel.Type("Model files", "*.txt");     //TODO: Decide on what we want to call them1

	if(!previous_model.IsEmpty() && FileExists(previous_model))
		model_sel.PreSelect(previous_model);
	
	model_sel.ExecuteOpen();
	model_file = model_sel.Get().ToStd();
	
	bool changed_model = model_file != previous_model.ToStd();
	
	if(model_file.empty()) {
		log("Received empty model file name.", true);
		return;
	}
	
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
	data_file = data_sel.Get().ToStd();
	
	if(data_file.empty()){
		log("Received empty data file name.", true);
		return;
	}
	
	bool changed_data_path = GetFileDirectory(data_file.data()) != GetFileDirectory(previous_data);

	log(Format("Selecting data file: %s", data_file.data()));
	

	bool success = true;
	try {
		model = load_model(model_file.data());
		model->compose();
		
		app = new Model_Application(model);
			
		data_set = new Data_Set;
		data_set->read_from_file(data_file.data());
		
		app->build_from_data_set(data_set);
	
		app->compile();
	} catch(int) {
		store_settings(false);
		success = false;
	}
	
	log_warnings_and_errors();

	if(!success)
		return;
	
	build_interface();
	store_settings(false);
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
	plotter.re_plot(true);
	//TODO!
	//if(OtherPlots.IsOpen())
	//	OtherPlots.BuildAll(true);
	//BaselineWasJustSaved = false;
}

void MobiView2::run_model() {
	if(!model_is_loaded()) {
		log("The model can only be run if it is loaded along with a data file.", true);
		return;
	}
	
	try {
		auto begin = std::chrono::high_resolution_clock::now();
		::run_model(app, -1);
		auto end = std::chrono::high_resolution_clock::now();
		double ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
		
		log(Format("Model was run.\nDuration: %g ms.", ms ));
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
	/*
	JsonArray IdxEditWindowDim;
	IdxEditWindowDim << ChangeIndexes.GetSize().cx << ChangeIndexes.GetSize().cy;
	SettingsJson("Index set editor window dimensions", IdxEditWindowDim);
	
	JsonArray AdditionalPlotViewDim;
	AdditionalPlotViewDim << OtherPlots.GetSize().cx << OtherPlots.GetSize().cy;
	SettingsJson("Additional plot view dimensions", AdditionalPlotViewDim);
	*/
	
	/*
	Json Statistics;
	
	#define SET_SETTING(Handle, Name, Type) \
		Statistics(#Handle, StatSettings.Display##Handle);
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type)
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	JsonArray Percentiles;
	for(double Percentile : StatSettings.Percentiles) Percentiles << Percentile;
	Statistics("Percentiles", Percentiles);
	
	Statistics("Precision", StatSettings.Precision);
	Statistics("BFIParam", StatSettings.EckhardtFilterParam);
	Statistics("ShowInitial", StatSettings.ShowInitialValue);
	
	SettingsJson("Statistics", Statistics);
	*/
	
	SaveFile("settings.json", settings_json.ToString());
}

void MobiView2::clean_interface() {
	par_group_selecter.Clear();
	par_group_nodes.Clear();
	
	result_selecter.Clear();
	input_selecter.Clear();
	series_nodes.Clear();
	
	params.clean();
	plotter.clean();
	//TODO
}

void add_series_node(MobiView2 *window, TreeCtrl &selecter, Mobius_Model *model, Var_Id var_id, State_Variable *var, int top_node, std::unordered_map<Entity_Id, int, Hash_Fun<Entity_Id>> &nodes_compartment,
	std::unordered_map<Var_Location, int, Var_Location_Hash> &nodes_quantity, bool is_input = false) {
		
	//TODO: We should make our own ctrl. for the nodes to store the var_id and have the
	//"favorite" checkbox.
	
	// NOTE: This relies on dissolved variables being later in the list than what they are
	// dissolved in. This is the current functionality, but is implementation dependent on
	// Mobius 2...
	// We could easily make it independent by just updating nodes with names properly
	// later...
	
	//TODO: allow for some of these!
	if(
		   var->flags & State_Variable::Flags::f_in_flux
		|| var->flags & State_Variable::Flags::f_in_flux_neighbor
		|| var->flags & State_Variable::Flags::f_is_aggregate
		|| var->flags & State_Variable::Flags::f_invalid)
		return;
	
	bool dissolved = false;
	bool diss_conc = false;
	bool flux_to   = false;
	
	Var_Location loc = var->loc1;
	
	if(var->flags & State_Variable::Flags::f_dissolved_conc) {
		auto diss_in = model->state_vars[var->dissolved_conc];
		loc = diss_in->loc1;
		diss_conc = true;
	}
	
	if(var->type == Decl_Type::flux && !is_located(loc)) {
		if(is_located(var->loc2)) {
			loc = var->loc2;
			flux_to = true;
		} else
			return;     //TODO: maybe we should make a top node for it instead.
	}
	
	if(!is_located(loc) && !is_input) {
		window->log(Format("Unable to identify why a variable \"%s\" was not located.", str(var->name)), true);
		return;
	}
	
	int parent_id = top_node;
	if(is_located(loc)) {
		if(loc.n_dissolved == 0) {
			auto find = nodes_compartment.find(loc.compartment);
			if(find == nodes_compartment.end()) {
				auto comp = model->find_entity<Reg_Type::compartment>(loc.compartment);
				window->series_nodes.Create(invalid_var, str(comp->name));
				parent_id = selecter.Add(top_node, IconImg::Compartment(), window->series_nodes.Top());//, false);
				selecter.SetNode(parent_id, selecter.GetNode(parent_id).CanSelect(false));
				nodes_compartment[loc.compartment] = parent_id;
			} else
				parent_id = find->second;
			
		} else {
			dissolved = true;
			Var_Location parent_loc = loc;
			if(!diss_conc && var->type != Decl_Type::flux)
				parent_loc = remove_dissolved(loc);
			
			auto find = nodes_quantity.find(parent_loc);
			if(find == nodes_quantity.end()) {
				window->log("Something went wrong with looking up quantities", false);
				return;
			}
			parent_id = find->second;
		}
	}
	
	String name;
	
	if(is_input) {
		name = str(var->name);
	} else if(diss_conc) {
		name = "concentration";
	} else if(var->type == Decl_Type::flux) {
		// NOTE: we don't want to use the generated name here, only the name of the base flux
		auto var2 = var;
		while(var2->flags & State_Variable::Flags::f_dissolved_flux)
			var2 = model->state_vars[var2->dissolved_flux];
		
		name = str(var2->name);
		//TODO: aggregate fluxes!
	} else {
		auto quant = model->find_entity<Reg_Type::property_or_quantity>(loc.property_or_quantity);
		name = str(quant->name);
	}

	Image *img;
	if(var->type == Decl_Type::quantity) {
		img = dissolved ? &IconImg::Dissolved() : &IconImg::Quantity();
	} else if(var->type == Decl_Type::property || is_input) {
		img = diss_conc ? &IconImg::DissolvedConc() : &IconImg::Property();
	} else if(var->type == Decl_Type::flux) {
		img = flux_to ? &IconImg::FluxTo() : &IconImg::Flux();
	}
	
	window->series_nodes.Create(var_id, name);
	int id = selecter.Add(parent_id, *img, window->series_nodes.Top());//, false);
	
	if(var->type == Decl_Type::quantity && is_located(loc))
		nodes_quantity[loc] = id;
}

void MobiView2::build_interface() {
	if(!model_is_loaded()) {
		log("Tried to build the interface without a model having been loaded", true);
		return;
	}
	
	par_group_selecter.Set(0, str(model->model_name));
	
	int module_idx = 0;
	for(auto mod : model->modules) {
		int id = 0;
		if(module_idx > 0) {
			String name = Format("%s v. %d.%d.%d", str(mod->module_name), mod->version.major, mod->version.minor, mod->version.revision);
			id = par_group_selecter.Add(0, Null, name, false);
			par_group_selecter.SetNode(id, par_group_selecter.GetNode(id).CanSelect(false));
		}
		
		for(auto par_group_id : mod->par_groups) {
			auto par_group = mod->par_groups[par_group_id];
			par_group_nodes.Create(par_group_id, str(par_group->name));
			par_group_selecter.Add(id, Null, par_group_nodes.Top(), false);
		}
		
		++module_idx;
	}
	par_group_selecter.OpenDeep(0, true);
	
	std::unordered_map<Entity_Id, int, Hash_Fun<Entity_Id>> nodes_compartment;
	std::unordered_map<Var_Location, int, Var_Location_Hash> nodes_quantity;

	try {
		for(Var_Id var_id : model->state_vars) {
			auto var = model->state_vars[var_id];
			if(var->type == Decl_Type::quantity)
				add_series_node(this, result_selecter, model, var_id, var, 0, nodes_compartment, nodes_quantity);
		}
		
		for(Var_Id var_id : model->state_vars) {
			auto var = model->state_vars[var_id];
			if(var->type == Decl_Type::property)
				add_series_node(this, result_selecter, model, var_id, var, 0, nodes_compartment, nodes_quantity);
		}
		
		for(Var_Id var_id : model->state_vars) {
			auto var = model->state_vars[var_id];
			if(var->type == Decl_Type::flux)
				add_series_node(this, result_selecter, model, var_id, var, 0, nodes_compartment, nodes_quantity);
		}
		
		nodes_compartment.clear();
		nodes_quantity.clear();
		
		int input_id = input_selecter.Add(0, Null, "Model inputs", false);
		int additional_id = input_selecter.Add(0, Null, "Additional series", false);
		
		input_selecter.SetNode(input_id, input_selecter.GetNode(input_id).CanSelect(false));
		input_selecter.SetNode(additional_id, input_selecter.GetNode(additional_id).CanSelect(false));
		
		for(Var_Id var_id : model->series) {
			auto var = model->series[var_id];
			add_series_node(this, input_selecter, model, var_id, var, input_id, nodes_compartment, nodes_quantity, true);
		}
		
		
		for(Var_Id var_id : app->additional_series) {
			auto var = app->additional_series[var_id];
			add_series_node(this, input_selecter, model, var_id, var, additional_id, nodes_compartment, nodes_quantity, true);
		}
		
	} catch (int) {}
	log_warnings_and_errors();
	
	result_selecter.OpenDeep(0, true);
	input_selecter.OpenDeep(0, true);
	
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
	
	if(app) delete app;
	// These two are not strictly necessary, but we should delete the app since otherwise jit
	// memory is not properly cleaned.
	if(data_set) delete data_set;
	if(model) delete model;
	
	Close();
}




GUI_APP_MAIN
{
	StdLogSetup(LOG_FILE, "MobiView2.log");
	MobiView2().Run();
}
