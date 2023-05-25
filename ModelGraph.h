#ifndef _MobiView2_ModelGraph_h_
#define _MobiView2_ModelGraph_h_

#include <CtrlLib/CtrlLib.h>

struct Model_Application;

class ModelGraph : public Upp::Ctrl {
	
public :
	ModelGraph() {};
	
	Upp::Image image;
	
	virtual void Paint(Upp::Draw& w) override;
	
	void rebuild_image(Model_Application *app);
	
	bool show_properties = false;
	bool show_flux_labels = true;
	bool show_short_names = false;
};


#endif
