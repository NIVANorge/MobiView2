#include "MobiView2.h"
#include "SeriesSelecter.h"


#define IMAGECLASS IconImg47
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

SeriesSelecter::SeriesSelecter(MobiView2 *parent, String root, Var_Id::Type type) : parent(parent), type(type) {
	CtrlLayout(*this);
	show_favorites.Disable();
	
	// TODO: We should intercept the selection and set it to all the trees (where applicable)
	// Also tab change should give plot change.
	
	var_tree.WhenSel << [=,this]() { parent->plotter.plot_change(); };
	var_tree.MultiSelect();
	var_tree.SetRoot(Null, String(root));
	var_tree.HighlightCtrl(true);
	var_tree.SetNode(0, var_tree.GetNode(0).CanSelect(false));
	
	quant_tree.WhenSel << [=,this]() { parent->plotter.plot_change(); };
	quant_tree.MultiSelect();
	quant_tree.SetRoot(Null, String(root));
	quant_tree.HighlightCtrl(true);
	quant_tree.SetNode(0, var_tree.GetNode(0).CanSelect(false));
	
	if(type == Var_Id::Type::state_var) {
		tree_tab.Add(var_tree.SizePos(), "Comp.");//IconImg47::Compartment(), "Comp.");
		tree_tab.Add(quant_tree.SizePos(), "Quant."); //IconImg47::Quantity(), "Quant.");
	} else {
		tree_tab.Disable();
		tree_tab.Hide();
		Add(var_tree.HSizePosZ(0, 0).VSizePosZ(25, 20));
	}
}

void
SeriesSelecter::clean() {
	var_tree.Clear();
	quant_tree.Clear();
	nodes.Clear();
}

void
SeriesSelecter::get_selected(std::vector<Var_Id> &push_to) {
	TreeCtrl *tree = &var_tree;
	
	int tab = tree_tab.GetData();
	if(type == Var_Id::Type::state_var) {
		if(tab == 1)
			tree = &quant_tree;
	}
	
	Vector<int> selected = tree->GetSel();
	for(int idx : selected) {
		Upp::Ctrl *ctrl = ~tree->GetNode(idx).ctrl;
		if(!ctrl) continue;
		push_to.push_back(reinterpret_cast<Entity_Node *>(ctrl)->var_id);
	}
}

void
recursive_select(TreeCtrl &tree, int node, const std::vector<Var_Id> &select) {
	Upp::Ctrl *ctrl = ~tree.GetNode(node).ctrl;
	if(ctrl) {
		Var_Id var_id = reinterpret_cast<Entity_Node *>(ctrl)->var_id;
		if(std::find(select.begin(), select.end(), var_id) != select.end())
			tree.SelectOne(node, true);
	}
	for(int child_idx = 0; child_idx < tree.GetChildCount(node); ++child_idx) {
		int child = tree.GetChild(node, child_idx);
		recursive_select(tree, child, select);
	}
}

void
SeriesSelecter::set_selection(const std::vector<Var_Id> &sel) {
	var_tree.ClearSelection();
	recursive_select(var_tree, 0, sel);
	
	quant_tree.ClearSelection();
	recursive_select(quant_tree, 0, sel);
}

Var_Location
reorder(const Var_Location &loc, bool actually_reorder, int len, bool inverse) {
	Var_Location result = loc;
	result.n_components = len;
	if(!actually_reorder)
		return result;
	for(int idx = 0; idx < len; ++idx) {
		int idx2;
		if(inverse)
			idx2 = (idx + 1) % len;
		else
			idx2 = (loc.n_components - 1 + idx) % loc.n_components;
		result.components[idx] = loc.components[idx2];
	}
	return result;
}

int
make_branch(TreeCtrl &tree, const Var_Location &loc, Mobius_Model *model, Var_Registry *reg,
	std::unordered_map<Var_Location, int, Var_Location_Hash> &loc_to_node, Array<Entity_Node> &nodes, int root, bool by_quantity) {
	int at = root;
	
	for(int len = 1; len <= loc.n_components; ++len) {
		Var_Location at_loc = reorder(loc, by_quantity, len, false);
	
		auto find = loc_to_node.find(at_loc);
		if(find == loc_to_node.end()) {
			auto comp_id = at_loc.components[len-1];
			auto comp = model->components[comp_id];
			
			auto var_id = reg->id_of(reorder(at_loc, by_quantity, len, true));
			nodes.Create(var_id, comp->name.data());
			Image *img;
			if(comp->decl_type == Decl_Type::compartment)
				img = &IconImg47::Compartment();
			else if(comp->decl_type == Decl_Type::quantity) {
				img = len>2 ? &IconImg47::Dissolved() : &IconImg47::Quantity();
			} else if (comp->decl_type == Decl_Type::property)
				img = &IconImg47::Property();
			
			at = tree.Add(at, *img, nodes.Top());
			if(!is_valid(var_id))
				tree.SetNode(at, tree.GetNode(at).CanSelect(false));
			loc_to_node[at_loc] = at;
		} else {
			at = find->second;
		}
	}
	return at;
}



void
add_series_node(MobiView2 *window, TreeCtrl &tree, Array<Entity_Node> &nodes, Model_Application *app, Var_Id var_id,
	int root, std::unordered_map<Var_Location, int, Var_Location_Hash> &loc_to_node, int pass_type, bool by_quantity = false) {

	Var_Registry *reg = nullptr;
	bool is_input = false;
	if(var_id.type == Var_Id::Type::state_var)
		reg = &app->state_vars;
	else if(var_id.type == Var_Id::Type::series) {
		is_input = true;
		reg = &app->series;
	} else if(var_id.type == Var_Id::Type::additional_series) {
		is_input = true;
		reg = &app->additional_series;
	}
	auto var = (*reg)[var_id];
	
	//TODO: allow regular aggregate
	if(
		    var->type == State_Var::Type::regular_aggregate
		|| !var->is_valid())
		return;
	
	// Just the phases we add nodes in not to have them jumbled. Also, we add quantities first
	// to organize other things by them.
	if(!is_input) {
		if(pass_type == 0) {
			// Do all declared quantities
			if(var->type != State_Var::Type::declared) return;
			if(as<State_Var::Type::declared>(var)->decl_type != Decl_Type::quantity) return;
		} else if (pass_type == 1) {
			// Do all declared properties, or things that are generated (but not fluxes)
			//TODO: Concentrations should go in their own pass between here.
			if(var->is_flux()) return;
			if(var->type == State_Var::Type::declared &&
				as<State_Var::Type::declared>(var)->decl_type != Decl_Type::property) return;
		} else if (pass_type == 2) {
			// Do all fluxes (declared and generated)
			if(!var->is_flux()) return;
		}
	}
		
	bool dissolved = false;
	bool diss_conc = false;
	bool flux_to   = false;
	bool connection_agg = false;
	bool in_flux_agg = false;
	
	Var_Location loc = var->loc1;
	
	if(var->type == State_Var::Type::dissolved_conc) {
		auto var2 = as<State_Var::Type::dissolved_conc>(var);
		loc = app->state_vars[var2->conc_of]->loc1;
		diss_conc = true;
	}
	
	if(var->type == State_Var::Type::connection_aggregate) {
		auto var2 = as<State_Var::Type::connection_aggregate>(var);
		loc = app->state_vars[var2->agg_for]->loc1;
		connection_agg = true;
	}
	
	if(var->type == State_Var::Type::in_flux_aggregate) {
		auto var2 = as<State_Var::Type::in_flux_aggregate>(var);
		loc = app->state_vars[var2->in_flux_to]->loc1;
		in_flux_agg = true;
	}
	
	if(var->is_flux() && !is_located(loc)) {
		if(is_located(var->loc2)) {
			loc = var->loc2;
			flux_to = true;
		} else {
			window->log(Format("The flux \"%s\" does not have a location in either end.", var->name.data()), true);
			return;
		}
	}
	
	if(!is_located(loc) && !is_input) {
		window->log(Format("Unable to identify why a variable \"%s\" was not located.", var->name.data()), true);
		return;
	}
	
	int at = root;
	if(is_located(loc))
		at = make_branch(tree, loc, app->model, reg, loc_to_node, nodes, root, by_quantity);
	
	// If we are a quantity or a property, the make_branch call took care of adding the node.
	if(var->type == State_Var::Type::declared && !var->is_flux() && is_located(loc))
		return;
	
	
	String name;
	if(connection_agg) {
		auto var2 = as<State_Var::Type::connection_aggregate>(var);
		auto conn = app->model->connections[var2->connection];
		if(var2->is_source)
			name = "to connection (" + conn->name + ")";
		else
			name = "from connection (" + conn->name + ")";
	} else if(in_flux_agg) {
		name = "total in (excluding connections)";
	} else if(is_input) {
		name = var->name;
	} else if(diss_conc) {
		name = "concentration";
	} else if(var->is_flux()) {
		// NOTE: we don't want to use the generated name here, only the name of the base flux
		auto var2 = var;
		while(var2->type == State_Var::Type::dissolved_flux)
			var2 = app->state_vars[as<State_Var::Type::dissolved_flux>(var2)->flux_of_medium];
		
		name = var2->name;
		//TODO: (other) aggregate fluxes!
	}

	Image *img = nullptr;
	if(connection_agg) {
		auto var2 = as<State_Var::Type::connection_aggregate>(var);
		img = var2->is_source ? &IconImg47::ConnectionFlux() : &IconImg47::ConnectionFluxTo();
	} else if(in_flux_agg) {
		img = &IconImg47::ConnectionFluxTo();
	} else if(var->is_flux()) {
		img = flux_to ? &IconImg47::FluxTo() : &IconImg47::Flux();
	} else  {
		img = diss_conc ? &IconImg47::DissolvedConc() : &IconImg47::Property();
	}
	
	nodes.Create(var_id, name);
	tree.Add(at, *img, nodes.Top());
}

void
SeriesSelecter::build(Model_Application *app) {
	std::unordered_map<Var_Location, int, Var_Location_Hash> loc_to_node;
	
	try {
		if(type == Var_Id::Type::state_var) {
			for(int pass = 0; pass < 3; ++pass) {
				for(Var_Id var_id : app->state_vars)
					add_series_node(parent, var_tree, nodes, app, var_id, 0, loc_to_node, pass);
			}
			
			loc_to_node.clear();
			
			for(int pass = 0; pass < 3; ++pass) {
				for(Var_Id var_id : app->state_vars)
					add_series_node(parent, quant_tree, nodes, app, var_id, 0, loc_to_node, pass, true);
			}
			
		} else {

			int input_id = var_tree.Add(0, Null, "Model inputs", false);
			int additional_id = var_tree.Add(0, Null, "Additional series", false);
			
			var_tree.SetNode(input_id, var_tree.GetNode(input_id).CanSelect(false));
			var_tree.SetNode(additional_id, var_tree.GetNode(additional_id).CanSelect(false));
			
			for(Var_Id var_id : app->series)
				add_series_node(parent, var_tree, nodes, app, var_id, input_id, loc_to_node, 1);
			
			for(Var_Id var_id : app->additional_series)
				add_series_node(parent, var_tree, nodes, app, var_id, additional_id, loc_to_node, 1);
		}
		
	} catch (int) {}
	parent->log_warnings_and_errors();
	
	var_tree.OpenDeep(0, true);
	quant_tree.OpenDeep(0, true);
}