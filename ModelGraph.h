#ifndef _MobiView2_ModelGraph_h_
#define _MobiView2_ModelGraph_h_

#include <CtrlLib/CtrlLib.h>

class MobiView2;


class ModelGraph : public Upp::TopWindow {
	
	MobiView2 *parent;
	bool has_image = false;
	Upp::Image image;
	
public :
	ModelGraph(MobiView2 *parent);
	
	virtual void Paint(Upp::Draw& w) override;
	
	void rebuild_image();
};


#endif
