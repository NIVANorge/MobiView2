#ifndef _MobiView2_ParameterCtrl_h_
#define _MobiView2_ParameterCtrl_h_

constexpr int MAX_INDEX_SETS = 6;    //NOTE: This has to match the number of (currently) hard coded index set displays in the main window.

#include <CtrlLib/CtrlLib.h>

#define LAYOUTFILE <MobiView2/ParameterCtrl.lay>
#include <CtrlCore/lay.h>


class MobiView2;
struct Model_Application;


#include "common_types.h"
#include "support/parameter_editing.h"


class ParameterCtrl : public WithParameterCtrlLayout<Upp::ParentCtrl> {
	
public:
	typedef ParameterCtrl CLASSNAME;
	
	ParameterCtrl(MobiView2* parent);
	
	bool changed_since_last_save = false;
	
	Upp::Label     *index_set_name[MAX_INDEX_SETS]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	Upp::DropList  *index_list[MAX_INDEX_SETS];
	Upp::Option    *index_lock[MAX_INDEX_SETS];
	Upp::Option    *index_expand[MAX_INDEX_SETS];
	
	Upp::Array<Upp::Ctrl> parameter_controls;
	
	std::vector<Entity_Id> index_sets;
	std::vector<std::vector<Indexed_Parameter>> listed_pars;
	
	MobiView2 *parent;
	
	void refresh(bool values_only = false);
	void clean();
	void build_index_set_selecters(Model_Application *app);
	void expand_index_set_clicked(int idx);
	
	void parameter_edit(Indexed_Parameter par_data, Model_Application *app, Parameter_Value val);
	Indexed_Parameter get_selected_parameter();
	std::vector<Indexed_Parameter> get_all_parameters();
	
private :
	void set_locks(Indexed_Parameter &par);
};

#endif
