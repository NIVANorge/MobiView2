#ifndef _MobiView2_ParameterCtrl_h_
#define _MobiView2_ParameterCtrl_h_

constexpr int MAX_INDEX_SETS = 6;    //NOTE: This has to match the number of (currently) hard coded index set displays in the main window.

#include <CtrlLib/CtrlLib.h>

#define LAYOUTFILE <MobiView2/ParameterCtrl.lay>
#include <CtrlCore/lay.h>



class EmptyDisplay : public Upp::Display
{
	public :
		virtual void Paint(Upp::Draw& w, const Upp::Rect& r, const Upp::Value& q, Upp::Color ink, Upp::Color paper, Upp::dword style) const {
			Upp::Display::Paint(w, r, Upp::Null, ink, paper, style);
		}
};

class MobiView2;
struct Model_Application;


#include "common_types.h"
#include "parameter_editing.h"


class ParameterCtrl : public WithParameterCtrlLayout<Upp::ParentCtrl> {
	
public:
	typedef ParameterCtrl CLASSNAME;
	
	ParameterCtrl(MobiView2* parent);
	
	EmptyDisplay no_display;
	
	bool changed_since_last_save = false;
	
	Upp::Label     *index_set_name[MAX_INDEX_SETS]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	Upp::DropList  *index_list[MAX_INDEX_SETS];
	Upp::Option    *index_lock[MAX_INDEX_SETS];
	Upp::Option    *index_expand[MAX_INDEX_SETS];
	
	Upp::Array<Upp::Ctrl> parameter_controls;
	//std::vector<parameter_type> CurrentParameterTypes;
	
	std::vector<Entity_Id> index_sets;
	
	MobiView2 *parent;
	
	void refresh(bool values_only = false);
	void clean();
	void build_index_set_selecters(Model_Application *app);
	void expand_index_set_clicked(int idx);
	
	void parameter_edit(Indexed_Parameter par_data, Model_Application *app, Parameter_Value val);
};

#endif
