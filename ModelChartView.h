#ifndef _MobiView2_ModelChartView_h_
#define _MobiView2_ModelChartView_h_

#include "StructureLayout.h"
#include "ModelGraph.h"
#include <AutoScroller/AutoScroller.h>

class MobiView2;

class ModelChartView : public WithStructureViewLayout<TopWindow> {
public :
	typedef ModelChartView CLASSNAME;
	
	MobiView2 *parent;
	
	Upp::ParentCtrl model_graph_tab;
	AutoScroller   model_graph_scroll;
	ModelGraph     model_graph;
	Upp::Option    show_properties;
	Upp::Option    show_flux_labels;
	Upp::Option    short_names;
	
	ModelChartView(MobiView2 *parent);
	
	void rebuild();
};

#endif
