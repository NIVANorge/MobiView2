#include "MobiView2.h"
#include "AdditionalPlotView.h"


#define IMAGECLASS IconImg2
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

using namespace Upp;

MiniPlot::MiniPlot() {
	CtrlLayout(*this);
}

AdditionalPlotView::AdditionalPlotView(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "Additional plot view");
	Sizeable().Zoomable();
	
	Add(vert_split.VSizePos(25, 0));
	vert_split.Vert();

	edit_n_rows.NotNull();
	edit_n_rows.SetData(2);
	edit_n_rows.Max(MAX_ADDITIONAL_PLOTS);
	edit_n_rows.WhenAction << [this](){ n_rows_changed(true); };
	
	link_all.NotNull();
	link_all.Set(1);
	link_all.WhenAction = THISBACK(update_link_status);
	
	AddFrame(tool);
	tool.Set(THISBACK(sub_bar));
	
	for(int row = 0; row < MAX_ADDITIONAL_PLOTS; ++row) {
		plots[row].copy_main.WhenPush << [this, row](){ copy_main_plot(row); };
		plots[row].push_main.WhenPush << [this, row](){
			this->parent->plotter.set_plot_setup(plots[row].plot.setup);
		};
		plots[row].plot_info.SetEditable(false);
		
		plots[row].plot.plot_info = &plots[row].plot_info;
		plots[row].plot.parent = parent;
	}
	
	override_list.Add((int)Plot_Mode::none, "(none)");
	override_list.Add((int)Plot_Mode::regular, "Regular");
	override_list.Add((int)Plot_Mode::stacked, "Stacked");
	override_list.Add((int)Plot_Mode::stacked_share, "Stacked share");
	override_list.Add((int)Plot_Mode::histogram, "Histogram");
	override_list.Add((int)Plot_Mode::profile, "Profile");
	override_list.Add((int)Plot_Mode::profile2D, "Profile 2D");
	override_list.Add((int)Plot_Mode::compare_baseline, "Compare baseline");
	override_list.Add((int)Plot_Mode::residuals, "Residuals");
	override_list.Add((int)Plot_Mode::residuals_histogram, "Residual histogram");
	override_list.Add((int)Plot_Mode::qq, "QQ");
	
	override_list.GoBegin();
	override_list.WhenAction << [this]{ build_all(); };
	
	n_rows_changed();
}

void AdditionalPlotView::sub_bar(Bar &bar) {
	bar.Add(IconImg2::Open(), THISBACK(read_from_json)).Tip("Load setup from file");
	bar.Add(IconImg2::Save(), THISBACK(write_to_json)).Tip("Save setup to file");
}

void AdditionalPlotView::n_rows_changed(bool rebuild) {
	int n_rows = edit_n_rows.GetData();
	
	if(IsNull(n_rows) || n_rows <= 0) return;
	vert_split.Clear();
	for(int row = 0; row < n_rows; ++row)
		vert_split.Add(plots[row]);
	
	if(rebuild) build_all();  //TODO: Technically only have to rebuild any newly added ones
}

void AdditionalPlotView::update_link_status() {
	for(int row = 0; row < MAX_ADDITIONAL_PLOTS; ++row)
		plots[row].plot.Unlinked();
	
	if(link_all.Get()) {
		int first_linkable = -1;
		for(int row = 0; row < MAX_ADDITIONAL_PLOTS; ++row) {
			Plot_Mode mode = plots[row].plot.setup.mode;
			if((int)override_list.GetData() != (int)Plot_Mode::none)
				mode = (Plot_Mode)(int)override_list.GetData();
			
			if(is_linkable(mode)) {
				if(first_linkable >= 0) {
					plots[row].plot.SetXYMin(plots[first_linkable].plot.GetXMin(), plots[row].plot.GetYMin());
					plots[row].plot.SetRange(plots[first_linkable].plot.GetXRange(), plots[row].plot.GetYRange());
					plots[row].plot.LinkedWith(plots[first_linkable].plot);
					plots[row].Refresh();
				}
				else
					first_linkable = row;
			}
		}
	}
}

void AdditionalPlotView::copy_main_plot(int row) {
	plots[row].plot.setup = parent->plotter.main_plot.setup;
	plots[row].plot.build_plot(false, (Plot_Mode)(int)override_list.GetData());
	update_link_status();
}

void AdditionalPlotView::build_all(bool caused_by_run) {
	int n_rows = edit_n_rows.GetData();
	for(int row = 0; row < n_rows; ++row)
		plots[row].plot.build_plot(caused_by_run, (Plot_Mode)(int)override_list.GetData());
	update_link_status();
}

void AdditionalPlotView::clean() {
	for(int row = 0; row < MAX_ADDITIONAL_PLOTS; ++row)
		plots[row].plot.clean();
}

void AdditionalPlotView::set_all(std::vector<Plot_Setup> &setups) {
	int count = std::min(MAX_ADDITIONAL_PLOTS, (int)setups.size());
	for(int row = 0; row < count; ++row)
		plots[row].plot.setup = setups[row];
	edit_n_rows.SetData(count);
	n_rows_changed(true);
}

void AdditionalPlotView::read_from_json() {
	PromptOK("Loading setup not yet implemented!"); //TODO!
}

void AdditionalPlotView::write_to_json() {
	PromptOK("Storing setup not yet implemented!"); //TODO!
}

/*
void AdditionalPlotView::SetAll(std::vector<plot_setup> &Setups)
{
	int Count = std::min((int)MAX_ADDITIONAL_PLOTS, (int)Setups.size());
	for(int Row = 0; Row < Count; ++Row)
		Plots[Row].Plot.PlotSetup = Setups[Row];
	EditNumRows.SetData(Count);
	NumRowsChanged(true);	
}

void SerializePlotSetup(Json &SetupJson, plot_setup &Setup, MobiView *ParentWindow)
{
	//NOTE: This is not robust if we change the enums!
	
	SetupJson("MajorMode", (int)Setup.MajorMode);
	SetupJson("AggregationType", (int)Setup.AggregationType);
	SetupJson("AggregationPeriod", (int)Setup.AggregationPeriod);
	SetupJson("YAxisMode", (int)Setup.YAxisMode);
	SetupJson("ProfileTimestep", Setup.ProfileTimestep);
	SetupJson("ScatterInputs", Setup.ScatterInputs);
	
	JsonArray ResultArr;
	for(std::string &R : Setup.SelectedResults)
		ResultArr << R.data();
	SetupJson("SelectedResults", ResultArr);
	
	JsonArray InputArr;
	for(std::string &R : Setup.SelectedInputs)
		InputArr << R.data();
	SetupJson("SelectedInputs", InputArr);
	
	Json IndexMap;
	int Id = -1;
	for(std::vector<std::string> &Arr : Setup.SelectedIndexes)
	{
		++Id;
		String IndexSetName = ParentWindow->IndexSetName[Id]->GetText();
		if(IndexSetName.IsEmpty()) continue;
		
		JsonArray InnerArr;
		for(std::string &Index : Arr)
			InnerArr << Index.data();
		IndexMap(IndexSetName, InnerArr);
		
		
	}
	SetupJson("SelectedIndexes", IndexMap);
	
	Json ActiveIndexSet;
	Id = -1;
	for(bool IsActive : Setup.IndexSetIsActive)
	{
		++Id;
		String IndexSetName = ParentWindow->IndexSetName[Id]->GetText();
		if(IndexSetName.IsEmpty()) continue;
		
		ActiveIndexSet(IndexSetName, IsActive);
	}
	SetupJson("IndexSetIsActive", ActiveIndexSet);
}


void AdditionalPlotView::WriteToJson()
{
	FileSel Sel;

	Sel.Type("Plot setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteSaveAs();
	String Filename = Sel.Get();
	
	if(Filename.GetLength() == 0) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store plot setups.");
		return;
	}
	
	Json MainFile;
	
	int NRows = EditNumRows.GetData();
	
	JsonArray Arr;
	
	for(int Row = 0; Row < NRows; ++Row)
	{
		Json RowSettings;
		SerializePlotSetup(RowSettings, Plots[Row].Plot.PlotSetup, ParentWindow);
		Arr << RowSettings;
	}
	
	MainFile("Setups", Arr);
	
	bool Lock = LinkAll.Get();
	
	MainFile("LinkAll", Lock);
	
	SaveFile(Filename, MainFile.ToString());
}


void DeserializePlotSetup(ValueMap &SetupJson, plot_setup &Setup, MobiView *ParentWindow)
{
	if(IsNull(SetupJson)) return;
	
	Value MajorMode = SetupJson["MajorMode"];
	if(!IsNull(MajorMode)) Setup.MajorMode = (plot_major_mode)(int)MajorMode;
	
	Value AggregationType = SetupJson["AggregationType"];
	if(!IsNull(AggregationType)) Setup.AggregationType = (aggregation_type)(int)AggregationType;
	
	Value AggregationPeriod = SetupJson["AggregationPeriod"];
	if(!IsNull(AggregationPeriod)) Setup.AggregationPeriod = (aggregation_period)(int)AggregationPeriod;
	
	Value YAxisMode = SetupJson["YAxisMode"];
	if(!IsNull(YAxisMode)) Setup.YAxisMode = (y_axis_mode)(int)YAxisMode;
	
	Value ScatterInputs = SetupJson["ScatterInputs"];
	if(!IsNull(ScatterInputs)) Setup.ScatterInputs = (bool)ScatterInputs;
	
	Value ProfileTimestep = SetupJson["ProfileTimestep"];
	if(!IsNull(ProfileTimestep)) Setup.ProfileTimestep = (int)ProfileTimestep;
	
	Setup.SelectedResults.clear();
	ValueArray SelectedResults = SetupJson["SelectedResults"];
	if(!IsNull(SelectedResults))
	{
		for(String Result : SelectedResults)
			Setup.SelectedResults.push_back(Result.ToStd());
	}
	
	Setup.SelectedInputs.clear();
	ValueArray SelectedInputs = SetupJson["SelectedInputs"];
	if(!IsNull(SelectedInputs))
	{
		for(String Input : SelectedInputs)
			Setup.SelectedInputs.push_back(Input.ToStd());
	}
	
	ValueMap SelectedIndexes = SetupJson["SelectedIndexes"];
	if(!IsNull(SelectedIndexes))
	{
		Setup.SelectedIndexes.resize(MAX_INDEX_SETS);
		for(int IndexSet = 0; IndexSet < SelectedIndexes.GetCount(); ++IndexSet)
		{
			Setup.SelectedIndexes[IndexSet].clear();
			String IndexSetName = ParentWindow->IndexSetName[IndexSet]->GetText();
			
			ValueArray Indexes = SelectedIndexes[IndexSetName];
			if(!IsNull(Indexes))
			{
				for(String Index : Indexes)
					Setup.SelectedIndexes[IndexSet].push_back(Index.ToStd());
			}
		}
	}
	
	Setup.IndexSetIsActive.clear();
	ValueMap IndexSetIsActive = SetupJson["IndexSetIsActive"];
	if(!IsNull(IndexSetIsActive))
	{
		for(int IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
		{
			String IndexSetName = ParentWindow->IndexSetName[IndexSet]->GetText();
			Value IsActive = IndexSetIsActive[IndexSetName];
			bool IsActive2 = false;
			if(!IsNull(IsActive)) IsActive2 = IsActive;
			Setup.IndexSetIsActive.push_back(IsActive2);
		}
	}
}

void AdditionalPlotView::LoadFromJson()
{
	FileSel Sel;
	
	Sel.Type("Plot setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteOpen();
	String Filename = Sel.Get();
	
	if(!FileExists(Filename)) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store plot setups.");
		return;
	}
	
	String SetupFile = LoadFile(Filename);
	
	Value SetupJson  = ParseJSON(SetupFile);
	
	Value Lock = SetupJson["LinkAll"];
	if(!IsNull(Lock)) LinkAll.Set((int)(bool)Lock);
	
	
	ValueArray Setups = SetupJson["Setups"];
	if(!IsNull(Setups))
	{
		int Count = Setups.GetCount();
		
		EditNumRows.SetData(Count);
		NumRowsChanged(false);
		
		for(int Row = 0; Row < Count; ++Row)
		{
			ValueMap Setup = Setups[Row];
			
			DeserializePlotSetup(Setup, Plots[Row].Plot.PlotSetup, ParentWindow);
		}
		
	}
	
	BuildAll();
}
*/