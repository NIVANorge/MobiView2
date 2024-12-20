#ifndef _MobiView2_SearchWindow_h_
#define _MobiView2_SearchWindow_h_

#include <vector>
#include <CtrlLib/CtrlLib.h>

#include "common_types.h"

#define LAYOUTFILE <MobiView2/SearchWindow.lay>
#include <CtrlCore/lay.h>

class MobiView2;

class SearchWindow : public WithSearchLayout<Upp::TopWindow>
{
public:
	typedef SearchWindow CLASSNAME;
	
	SearchWindow(MobiView2 *parent);
	
	MobiView2 *parent;
	
	void clean();
	void find();
	void select_item();
	
private:
	std::vector<std::pair<Entity_Id, Entity_Id>> par_list;
};

#endif
