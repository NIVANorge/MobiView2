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

void
AdditionalPlotView::sub_bar(Bar &bar) {
	bar.Add(IconImg2::Open(), THISBACK(read_from_json)).Tip("Load setup from file");
	bar.Add(IconImg2::Save(), THISBACK(write_to_json)).Tip("Save setup to file");
}

void
AdditionalPlotView::n_rows_changed(bool rebuild) {
	int n_rows = edit_n_rows.GetData();
	
	if(IsNull(n_rows) || n_rows <= 0) return;
	vert_split.Clear();
	for(int row = 0; row < n_rows; ++row)
		vert_split.Add(plots[row]);
	
	if(rebuild) build_all();  //TODO: Technically only have to rebuild any newly added ones
}

void
AdditionalPlotView::update_link_status() {
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

void
AdditionalPlotView::copy_main_plot(int row) {
	plots[row].plot.setup = parent->plotter.main_plot.setup;
	plots[row].plot.build_plot(false, (Plot_Mode)(int)override_list.GetData());
	update_link_status();
}

void
AdditionalPlotView::build_all(bool caused_by_run) {
	int n_rows = edit_n_rows.GetData();
	for(int row = 0; row < n_rows; ++row)
		plots[row].plot.build_plot(caused_by_run, (Plot_Mode)(int)override_list.GetData());
	update_link_status();
}

void
AdditionalPlotView::clean() {
	for(int row = 0; row < MAX_ADDITIONAL_PLOTS; ++row)
		plots[row].plot.clean();
}

void
AdditionalPlotView::set_all(std::vector<Plot_Setup> &setups) {
	int count = std::min(MAX_ADDITIONAL_PLOTS, (int)setups.size());
	for(int row = 0; row < count; ++row)
		plots[row].plot.setup = setups[row];
	edit_n_rows.SetData(count);
	n_rows_changed(true);
}

Vector<String>
AdditionalPlotView::serialize_setups() {
	Vector<String> result;
	
	int count = std::min((int)edit_n_rows.GetData(), MAX_ADDITIONAL_PLOTS);
	for(int row = 0; row < count; ++row)
		result << serialize_plot_setup(parent->app, plots[row].plot.setup);
	
	return std::move(result);
}


void
AdditionalPlotView::write_to_json() {
	
	if(!parent->model_is_loaded()) return;
	
	FileSel sel;

	sel.Type("Plot setups", "*.json");
	
	if(!parent->data_file.empty())
		sel.ActiveDir(GetFileFolder(parent->data_file.data()));
	else
		sel.ActiveDir(GetCurrentDirectory());
	
	sel.ExecuteSaveAs();
	String file_name = sel.Get();
	
	if(file_name.GetLength() == 0) return;
	
	if(GetFileName(file_name) == "settings.json") {
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store plot setups.");
		return;
	}
	
	Json main_file;
	
	JsonArray setups;
	int n_rows = edit_n_rows.GetData();
	for(int row = 0; row < n_rows; ++row) {
		Json row_setup;
		serialize_plot_setup(parent->app, row_setup, plots[row].plot.setup);
		setups << row_setup;
	}
	main_file("setups", setups);
	main_file("link_all", (bool)link_all.Get());
	main_file("override_mode", (int)override_list.GetData());
	
	SaveFile(file_name, main_file.ToString());
}

void AdditionalPlotView::read_from_json() {
	FileSel sel;
	
	sel.Type("Plot setups", "*.json");
	
	if(!parent->data_file.empty())
		sel.ActiveDir(GetFileFolder(parent->data_file.data()));
	else
		sel.ActiveDir(GetCurrentDirectory());
	
	sel.ExecuteOpen();
	String file_name = sel.Get();
	
	if(!FileExists(file_name)) return;
	
	if(GetFileName(file_name) == "settings.json") {
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store plot setups.");
		return;
	}
	
	String setup_file = LoadFile(file_name);
	Value setup_json  = ParseJSON(setup_file);
	
	Value link = setup_json["link_all"];
	if(!IsNull(link)) link_all.Set((int)(bool)link);
	
	Value override_mode = setup_json["override_mode"];
	if(!IsNull(override_mode)) override_list.SetData((int)override_mode);
	
	ValueArray setups_json = setup_json["setups"];
	if(!IsNull(setups_json)) {
		int count = setups_json.GetCount();
		
		std::vector<Plot_Setup> plot_setups;
		
		for(int row = 0; row < count; ++row) {
			Value setup = setups_json[row];
			plot_setups.push_back(deserialize_plot_setup(parent->app, setup));
		}
		
		set_all(plot_setups);
	} else {
		edit_n_rows.SetData(1);
		//TODO: Should set the setup in plot 1 to blank.
		n_rows_changed(true);
	}
}
