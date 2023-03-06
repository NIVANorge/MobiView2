#ifndef _MobiView2_StructureView_h_
#define _MobiView2_StructureView_h_

#define LAYOUTFILE <MobiView2/StructureView.lay>
#include <CtrlCore/lay.h>

class MobiView2;

class StructureViewWindow : public WithStructureViewLayout<TopWindow>
{
public:
	typedef StructureViewWindow CLASSNAME;
	
	StructureViewWindow(MobiView2 *parent);
	
	MobiView2 *parent;
	
	void refresh_text();
	
	Upp::DocEdit batch_structure, function_tree, llvm_ir;
};

#endif
