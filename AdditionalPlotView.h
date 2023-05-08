#ifndef _MobiView2_AdditionalPlotView_h_
#define _MobiView2_AdditionalPlotView_h_

#include <CtrlLib/CtrlLib.h>
#include "MyRichView.h"
#include "PlotCtrl.h"

#define LAYOUTFILE <MobiView2/AdditionalPlotView.lay>
#include <CtrlCore/lay.h>

constexpr int MAX_ADDITIONAL_PLOTS = 10;

class MiniPlot : public WithMiniPlotLayout<Upp::ParentCtrl> {
public:
	typedef MiniPlot CLASSNAME;
	
	MiniPlot();
};

class AdditionalPlotView : public WithAdditionalPlotViewLayout<Upp::TopWindow> {
public:
	typedef AdditionalPlotView CLASSNAME;
	
	AdditionalPlotView(MobiView2 *parent);
	
	void copy_main_plot(int row);
	
	void build_all(bool caused_by_run = false);
	void clean();
	
	void n_rows_changed(bool rebuild = true);
	void update_link_status();
	
	void write_to_json();
	void read_from_json();
	
	void set_all(std::vector<Plot_Setup> &setups);
	
	Upp::Vector<Upp::String> serialize_setups();
	
private:
	MobiView2 *parent;
	
	Upp::Splitter vert_split;
	MiniPlot plots[MAX_ADDITIONAL_PLOTS];
	ToolBar tool;
	
	void sub_bar(Upp::Bar &bar);
};

#endif
