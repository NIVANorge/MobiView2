#ifndef _MobiView2_SensitivityViewWindow_h_
#define _MobiView2_SensitivityViewWindow_h_

#include "PlotCtrl.h"

#define LAYOUTFILE <MobiView2/SensitivityViewWindow.lay>
#include <CtrlCore/lay.h>

class StatPlotCtrl : public WithSensitivityStatPlotLayout<Upp::ParentCtrl> {
public:
	typedef StatPlotCtrl CLASSNAME;
	StatPlotCtrl();
};

class MobiView2;

#include "model_application.h"

class SensitivityViewWindow : public WithSensitivityLayout<Upp::TopWindow> {
public:
	typedef SensitivityViewWindow CLASSNAME;
	
	SensitivityViewWindow(MobiView2 *parent);
	
	void update();
	void run();
	
private :
	MobiView2 *parent;
	Indexed_Parameter par;
	
	Upp::Splitter     main_horizontal;
	MyPlot       plot;
	StatPlotCtrl stat_plot;
	
	std::unordered_map<Entity_Id, std::pair<double, double>, Hash_Fun<Entity_Id>> cached_min_max;
};

#endif
