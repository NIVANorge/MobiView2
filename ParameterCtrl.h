#ifndef _MobiView2_ParameterCtrl_h_
#define _MobiView2_ParameterCtrl_h_

constexpr int MAX_INDEX_SETS = 6;    //NOTE: This now just a guideline for when the UI may start to look weird if more than this is displayed.

#include <CtrlLib/CtrlLib.h>

#define LAYOUTFILE <MobiView2/ParameterCtrl.lay>
#include <CtrlCore/lay.h>


class MobiView2;
struct Model_Application;


#include "common_types.h"
#include "support/parameter_editing.h"

class
ParameterMapCtrl : public WithParameterMapEditLayout<Upp::TopWindow> {

public :
	typedef ParameterMapCtrl CLASSNAME;
	
	ParameterMapCtrl(MobiView2 *parent);
	
	void clean(bool clean_par = false);
	void refresh();
	void select_par(Indexed_Parameter &par);
	void add_pushed();
	void remove_pushed();
	void pos_update(int row, double value);
	void value_update(int row, double value);
	
private :
	MobiView2 *parent;
	
	Entity_Id current_par = invalid_entity_id;
	Entity_Id current_par_app = invalid_entity_id;
	Indexes   current_indexes;
	
	Upp::Array<Upp::EditDoubleNotNull> pos_ctrls;
	Upp::Array<Upp::EditDoubleNotNull> value_ctrls;
	std::vector<int>                   data_index;
	
	void propagate_edit();
	void show_at_row(int row_idx, int data_idx, Parmap_Entry &entry);
};



class ParameterCtrl : public WithParameterCtrlLayout<Upp::ParentCtrl> {
	
public:
	typedef ParameterCtrl CLASSNAME;
	
	ParameterCtrl(MobiView2* parent);
	
	bool changed_since_last_save = false;
	
	Upp::Array<Upp::Label>        index_set_names;
	Upp::Array<Upp::DropList>     index_lists;
	Upp::Array<Upp::ButtonOption> index_locks;
	Upp::Array<Upp::ButtonOption> index_expands;
	
	Upp::Array<Upp::Array<Upp::Ctrl>> parameter_controls;
	
	MobiView2 *parent;
	
	void par_group_change();
	void refresh_parameter_view(bool values_only = false);
	void clean();
	void build_index_set_selecters(Model_Application *app);
	void expand_index_set_clicked(Entity_Id id);
	void list_change(Entity_Id id);
	
	void parameter_edit(Indexed_Parameter par_data, Model_Application *app, Parameter_Value val);
	Indexed_Parameter get_selected_parameter();
	std::vector<Indexed_Parameter> get_all_parameters();
	std::vector<Entity_Id> get_row_parameters();
	
private :
	void set_locks(Indexed_Parameter &par);
	
	void switch_expanded(Entity_Id id, bool on, bool force=false);
	
	void set_single_parameter(Upp::ValueMap &row_data, Indexed_Parameter &par_data, Upp::Id &value_column, int row, int col, bool show_additional, bool values_only, bool can_edit, bool map_form);
	
	Entity_Id par_group_id = invalid_entity_id;
	std::vector<Entity_Id> index_sets;
	std::vector<std::vector<Indexed_Parameter>> listed_pars;
	
	int focus_row = -1;
	int focus_col = -1;
	
	Entity_Id expanded_row = invalid_entity_id;
	Entity_Id expanded_col = invalid_entity_id;
	
	ParameterMapCtrl map_edit;
};


#endif
