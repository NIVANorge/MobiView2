#include "MobiView2.h"
#include "ModelChartView.h"

ModelChartView::ModelChartView(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "MobiView2 model graph and index editing");
	Sizeable().Zoomable();
	
	model_graph_tab.Add(model_graph_scroll.HSizePosZ(0, 0).VSizePosZ(0, 30));
	
	model_graph_tab.Add(show_properties.LeftPos(5, 105).BottomPos(0, 30));
	show_properties.SetLabel("Show properties");
	show_properties.SetData(model_graph.show_properties);
	show_properties.WhenAction << THISBACK(rebuild);
	
	model_graph_tab.Add(show_flux_labels.LeftPos(110, 210).BottomPos(0, 30));
	show_flux_labels.SetLabel("Show flux labels");
	show_flux_labels.SetData(model_graph.show_flux_labels);
	show_flux_labels.WhenAction << THISBACK(rebuild);
	
	model_graph_tab.Add(short_names.LeftPos(215, 300).BottomPos(0, 30));
	short_names.SetLabel("Short names");
	short_names.SetData(model_graph.show_short_names);
	short_names.WhenAction << THISBACK(rebuild);
	
	view_tabs.Add(model_graph_tab.SizePos(), "Flux graph");

}

void
ModelChartView::rebuild() {
	
	model_graph.show_properties = show_properties.GetData();
	model_graph.show_flux_labels = show_flux_labels.GetData();
	model_graph.show_short_names = short_names.GetData();
	
	//if(!IsOpen()) return;
	// TODO: Only care about rebuilding visible tabs, then rebuild when changing tabs.
	try {
		if(!parent->model_is_loaded())
			model_graph.rebuild_image(nullptr);
		model_graph.rebuild_image(parent->app);
	} catch(int) {
		parent->log_warnings_and_errors();
	}
	model_graph_scroll.ClearPane();
	Size sz = model_graph.image.GetSize();
	model_graph_scroll.AddPane(model_graph.LeftPos(0, sz.cx).TopPos(0, sz.cy), sz);
}