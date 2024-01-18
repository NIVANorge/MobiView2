#ifndef _MobiView2_SeriesSelecter_h_
#define _MobiView2_SeriesSelecter_h_

#include <CtrlLib/CtrlLib.h>

#define LAYOUTFILE <MobiView2/SeriesSelecter.lay>
#include <CtrlCore/lay.h>

class MobiView2;

#include "model_application.h"

// TODO: should also have a star option for "favorites".
class Entity_Node : public Upp::Label {
public:
	typedef Entity_Node CLASSNAME;
	
	Entity_Node(Var_Id var_id, const Upp::String &name) : var_id(var_id) { SetText(name); }
	Entity_Node(Entity_Id entity_id, const Upp::String &name, int select_idx = -1) : entity_id(entity_id), select_idx(select_idx) { SetText(name); }
	
	Var_Id var_id = invalid_var;
	Entity_Id entity_id = invalid_entity_id;
	int select_idx = -1; // for quick_select
};

class SeriesSelecter : public WithSeriesSelecterLayout<Upp::ParentCtrl> {
	
public:
	typedef SeriesSelecter CLASSNAME;
	
	SeriesSelecter(MobiView2 *parent, String root, Var_Id::Type type);
	
	void clean();
	void build(Model_Application *app);
	void get_selected(std::vector<Var_Id> &push_to);
	void set_selection(const std::vector<Var_Id> &sel);
	void search_change();
	void show_flux_change();
	
	void context_menu(Bar &bar);
	void add_new_quick_select();
	
private:
	MobiView2 *parent;
	Array<Entity_Node> nodes;
	Upp::TreeCtrl    var_tree;
	Upp::TreeCtrl    quant_tree;
	Upp::TreeCtrl    quick_tree;
	
	Upp::TreeCtrl*   current_tree();
	
	Var_Id::Type     type;
};

#endif
