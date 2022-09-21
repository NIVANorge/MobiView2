#ifndef _MobiView2_ModelInfoWindow_h_
#define _MobiView2_ModelInfoWindow_h_

#include <CtrlLib/CtrlLib.h>

#include "MyRichView.h"

#define LAYOUTFILE <MobiView2/ModelInfoWindow.lay>
#include <CtrlCore/lay.h>

class MobiView2;

class ModelInfoWindow : public WithModelInfoLayout<Upp::TopWindow> {
public:
	typedef ModelInfoWindow CLASSNAME;
	
	ModelInfoWindow(MobiView2 *parent);

	void refresh_text();
	
	MobiView2 *parent;
};

#endif
