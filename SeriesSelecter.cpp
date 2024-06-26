#include "MobiView2.h"
#include "SeriesSelecter.h"


#define IMAGECLASS IconImg47
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

SeriesSelecter::SeriesSelecter(MobiView2 *parent, String root, Var_Id::Type type) : parent(parent), type(type) {
	CtrlLayout(*this);
	
	//show_favorites.Disable();
	
	// TODO: We should intercept the selection and set it to all the trees (where applicable)
	// Also tab change should give plot change.
	
	var_tree.WhenSel << [=]() { parent->plotter.plot_change(); };
	var_tree.MultiSelect();
	var_tree.SetNode(0, var_tree.GetNode(0).CanSelect(false).Set(root));
	var_tree.HighlightCtrl(true);
	
	quant_tree.WhenSel << [=]() { parent->plotter.plot_change(); };
	quant_tree.MultiSelect();
	quant_tree.SetNode(0, var_tree.GetNode(0).CanSelect(false).Set(root));
	quant_tree.HighlightCtrl(true);
	
	quick_tree.WhenSel << [=]() { parent->plotter.plot_change(); };
	quick_tree.MultiSelect();
	quick_tree.SetNode(0, quick_tree.GetNode(0).CanSelect(false).Set("Quick select"));
	quick_tree.HighlightCtrl(true);
	
	if(type == Var_Id::Type::state_var) {
		option_show_fluxes.SetData(true);
		option_show_fluxes.WhenAction << THISBACK(show_flux_change);
	} else
		option_show_fluxes.Hide();
	
	if(type == Var_Id::Type::state_var) {
		tree_tab.Add(var_tree.SizePos(), "By compartment");//IconImg47::Compartment(), "Comp.");
		tree_tab.Add(quant_tree.SizePos(), "By quantity"); //IconImg47::Quantity(), "Quant.");
		tree_tab.Add(quick_tree.SizePos(), "Quick select");
		search_bar.WhenAction << THISBACK(search_change);
		tree_tab.WhenSet << THISBACK(search_change);
	} else {
		tree_tab.Disable();
		tree_tab.Hide();
		Add(var_tree.HSizePosZ(0, 0).VSizePosZ(25, 20));
		search_bar.Disable();
		search_bar.Hide();
	}
	
	quick_tree.WhenBar << THISBACK(context_menu);
	
	open_close.WhenAction << [this]() {
		auto tree = current_tree();
		
		bool closed = !tree->IsOpen(0);
		int count = tree->GetChildCount(0);
		if(!closed && count > 0) {
			closed = true;
			for(int idx = 0; idx < count; ++idx) {
				if(tree->IsOpen(tree->GetChild(0, idx))) {
					closed = false;
					break;
				}
			}
		}
		
		if(closed) {
			tree->OpenDeep(0);
		} else {
			tree->CloseDeep(0);
			tree->Open(0);
		}	
	};
}

void
SeriesSelecter::context_menu(Bar &bar) {
	bar.Add("Add new", THISBACK(add_new_quick_select));
}

void
SeriesSelecter::add_new_quick_select() {
	
	// TODO: Add a different one per sub-tree depending on where clicked, instead?
	
	//parent->log("Clicked");
	auto data_set = parent->data_set;
	if(!data_set) return;
	std::vector<Var_Id> sel;
	
	// TODO: This doesn't work, because we are on the wrong tab :(
	//  Do we need to cache the sel every time there is a click?
	get_selected(sel);
	if(sel.empty()) return;
	// TODO: Prompt for the name?
	auto name = parent->app->vars[sel[0]]->name;
	auto quick_id = data_set->quick_selects.create_internal(&data_set->top_scope, "", name, Decl_Type::quick_select);
	auto quick = data_set->quick_selects[quick_id];
	Quick_Select qsel;
	qsel.name = name;
	for(auto var_id : sel)
		qsel.series_names.push_back(parent->app->vars[var_id]->name); // TODO: Or serialize?
	quick->selects.emplace_back(std::move(qsel));
	//TODO: Need to refresh the tree view.
}

void
SeriesSelecter::clean() {
	var_tree.Clear();
	quant_tree.Clear();
	quick_tree.Clear();
	nodes.Clear();
}

TreeCtrl *
SeriesSelecter::current_tree() {
	TreeCtrl *tree = &var_tree;
	
	int tab = tree_tab.GetData();
	if(type == Var_Id::Type::state_var) {
		if(tab == 1)
			tree = &quant_tree;
		if(tab == 2)
			tree = &quick_tree;
	}
	
	return tree;
}

void
SeriesSelecter::search_change() {
	std::string match = search_bar.GetData().ToStd();

	std::transform(match.begin(), match.end(), match.begin(), ::tolower); // TODO: Trim bounding whitespaces
	
	TreeCtrl *tree = current_tree();
	
	// Match vs the name of all the top level nodes.
	int count = tree->GetChildCount(0);
	for(int i = 0; i < count; ++i) {
		int nodeidx = tree->GetChild(0, i);
		Upp::Ctrl *ctrl = ~tree->GetNode(nodeidx).ctrl;
		if(!ctrl) continue; // Should not happen though
		std::string node_name = reinterpret_cast<Label *>(ctrl)->GetText().ToStd();
		std::transform(node_name.begin(), node_name.end(), node_name.begin(), ::tolower);
		size_t pos = node_name.find(match);	
		bool matches = match.empty() || (pos != std::string::npos);
		//parent->log(Format("%s matches %s : %d", node_name.c_str(), match.c_str(), matches));
		tree->Open(nodeidx, matches);
	}
}

void
SeriesSelecter::show_flux_change() {
	// Annoying that we can't hide/show individual rows.
	// TODO: Would be nice to retain what variables were selected and reselect them.
	if(!parent->model_is_loaded()) return;
	clean();
	build(parent->app);
}

void
SeriesSelecter::get_selected(std::vector<Var_Id> &push_to) {
	TreeCtrl *tree = current_tree();
	
	int tab = tree_tab.GetData();
	
	Vector<int> selected = tree->GetSel();
	for(int idx : selected) {
		Upp::Ctrl *ctrl = ~tree->GetNode(idx).ctrl;
		if(!ctrl) continue;
		if(tab == 2) { // TODO: Bad way to hard code it. Should be encoded with the node instead
			auto node = static_cast<Entity_Node *>(ctrl);
			
			auto quick_select = parent->data_set->quick_selects[node->entity_id];
			auto &select = quick_select->selects[node->select_idx];
			for(auto &series_name : select.series_names) {
				auto var_id = parent->app->deserialize(series_name);
				if(is_valid(var_id))
					push_to.push_back(var_id);
				// TODO: Else log warning?
			}
		} else {
			push_to.push_back(static_cast<Entity_Node *>(ctrl)->var_id);
		}
	}
}

/*         Begin working on making reloads better when quick_select is active.
void
SeriesSelecter::get_selected_quick_selects(std::vector<std::pair<std::string, std::string>> &push_to) {
	
	if(!parent->data_set) return;
	
	int tab = tree_tab.GetData();
	if(tab != 2) return; // TODO: Bad hard coding.
	
	TreeCtrl *tree = current_tree();
	Vector<int> selected = tree->GetSel();
	for(int idx : selected) {
		Upp::Ctrl *ctrl = ~tree->GetNode(idx).ctrl;
		if(!ctrl) continue;
		auto node = static_cast<Entity_Node *>(ctrl);
		auto quick_select = parent->data_set->quick_selects[node->entity_id];
		
		push_to.push_back({parent->data_set->serialize(node->entity_id), node->GetText().ToStd()});
	}
}

void
SeriesSelecter::set_selected_quick_selects(std::vector<std::pair<std::string, std::string>> &selects) {
	
	for(auto &sel : selects) {
	}	
}
*/

void
recursive_select(TreeCtrl &tree, int node, const std::vector<Var_Id> &select) {
	Upp::Ctrl *ctrl = ~tree.GetNode(node).ctrl;
	if(ctrl) {
		Var_Id var_id = static_cast<Entity_Node *>(ctrl)->var_id;
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
make_branch(TreeCtrl &tree, const Var_Location &loc, Model_Application *app, Var_Registry *reg,
	std::unordered_map<Var_Location, int, Var_Location_Hash> &loc_to_node, Array<Entity_Node> &nodes, int root, bool by_quantity, Var_Id::Type type) {
	int at = root;
	
	auto model = app->model;
	
	for(int len = 1; len <= loc.n_components; ++len) {
		Var_Location at_loc = reorder(loc, by_quantity, len, false);
	
		auto find = loc_to_node.find(at_loc);
		if(find == loc_to_node.end()) {
			Entity_Id comp_id_img;
			if(!by_quantity)
				comp_id_img = at_loc.last();
			else
				comp_id_img = at_loc.first();
			auto comp_img = model->components[comp_id_img];
			auto comp_name = model->components[at_loc.last()];
			
			auto var_id = reg->id_of(reorder(at_loc, by_quantity, len, true));
			nodes.Create(var_id, comp_name->name.data());
			
			
			Image *img;
			if(comp_img->decl_type == Decl_Type::compartment || (by_quantity && !is_valid(var_id)))
				img = &IconImg47::Compartment();
			else if(comp_img->decl_type == Decl_Type::quantity) {
				img = len>2 ? &IconImg47::Dissolved() : &IconImg47::Quantity();
			} else if (comp_img->decl_type == Decl_Type::property)
				img = &IconImg47::Property();
			
			at = tree.Add(at, *img, nodes.Top());
			if(!is_valid(var_id) || var_id.type != type || !app->vars[var_id]->store_series)
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
	int root, std::unordered_map<Var_Location, int, Var_Location_Hash> &loc_to_node, int pass_type, bool show_fluxes = false, bool by_quantity = false, int n_comp = 0) {

	bool is_input = (var_id.type == Var_Id::Type::series) || (var_id.type == Var_Id::Type::additional_series);
	auto var = app->vars[var_id];
	
	// TODO: Make this behaviour configurable: Or maybe escape it in dev mode always?
	if(!var->store_series) return;
	
	if(!show_fluxes && 
		(var->is_flux() || var->type == State_Var::Type::in_flux_aggregate || var->type == State_Var::Type::connection_aggregate))
		return;
	
	//TODO: allow regular aggregate
	if(
		    var->type == State_Var::Type::regular_aggregate
		||  var->type == State_Var::Type::parameter_aggregate
		||  var->type == State_Var::Type::external_computation
		||  var->type == State_Var::Type::step_resolution // These are handled separately.
		|| !var->is_valid())
		return;
	
	// Just the phases we add nodes in not to have them jumbled. Also, we add quantities first
	// to organize other things by them.
	bool is_property = false;
	
	if(!is_input) {
		if(pass_type == 0) {
			// Do all declared quantities
			if(var->type != State_Var::Type::declared) return;
			if(as<State_Var::Type::declared>(var)->decl_type != Decl_Type::quantity) return;
			
		} else if (pass_type == 1) {
			if(var->type != State_Var::Type::dissolved_conc) return;
		} else if (pass_type == 2) {
			// Do all declared properties, or things that are generated (but not fluxes or concentrations)
			if(var->is_flux()) return;
			if(var->type == State_Var::Type::dissolved_conc) return;
			if(var->type == State_Var::Type::declared) {
				if(as<State_Var::Type::declared>(var)->decl_type != Decl_Type::property) return;
				is_property = true;
			}
		} else if (pass_type == 3) {
			// Do all fluxes (declared and generated)
			if(!var->is_flux()) return;
		}
	}
		
	bool dissolved = false;
	bool diss_conc = false;
	bool flux_to   = false;
	bool connection_agg = false;
	bool in_flux_agg = false;
	bool in_flux_out = false;
	
	Var_Location loc = var->loc1;
	
	if(var->type == State_Var::Type::dissolved_conc) {
		auto var2 = as<State_Var::Type::dissolved_conc>(var);
		loc = app->vars[var2->conc_of]->loc1;
		diss_conc = true;
	}
	
	if(var->type == State_Var::Type::connection_aggregate) {
		auto var2 = as<State_Var::Type::connection_aggregate>(var);
		loc = app->vars[var2->agg_for]->loc1;
		connection_agg = true;
	}
	
	if(var->type == State_Var::Type::in_flux_aggregate) {
		auto var2 = as<State_Var::Type::in_flux_aggregate>(var);
		loc = app->vars[var2->in_flux_to]->loc1;
		in_flux_agg = true;
		in_flux_out = var2->is_out;
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
	
	if(n_comp > 0) {
		if((!is_property && (loc.n_components != n_comp)) || (is_property && (loc.n_components-1 != n_comp)))
			return;
	}
	
	int at = root;
	if(is_located(loc))
		at = make_branch(tree, loc, app, &app->vars, loc_to_node, nodes, root, by_quantity, var_id.type);
	
	// If we are a quantity or a property, the make_branch call took care of adding the node.
	if(var->type == State_Var::Type::declared && !var->is_flux() && is_located(loc))
		return;
	
	
	String name;
	if(connection_agg) {
		auto var2 = as<State_Var::Type::connection_aggregate>(var);
		auto conn = app->model->connections[var2->connection];
		if(var2->is_out)
			name = "to connection (" + conn->name + ")";
		else
			name = "from connection (" + conn->name + ")";
	} else if(in_flux_agg) {
		if(in_flux_out)
			name = "total out (excluding connections)";
		else
			name = "total in (excluding connections)";
	} else if(is_input) {
		name = var->name;
	} else if(diss_conc) {
		name = "concentration";
	} else if(var->is_flux()) {
		// NOTE: we don't want to use the generated name here, only the name of the base flux.
		auto var2 = app->vars[app->find_base_flux(var_id)];
		name = var2->name;
		//TODO: (other) aggregate fluxes!
	} else
		name = var->name;

	Image *img = nullptr;
	if(connection_agg) {
		auto var2 = as<State_Var::Type::connection_aggregate>(var);
		img = var2->is_out ? &IconImg47::ConnectionFlux() : &IconImg47::ConnectionFluxTo();
	} else if(in_flux_agg) {
		img = in_flux_out ? &IconImg47::ConnectionFlux() : &IconImg47::ConnectionFluxTo();
	} else if(var->is_flux()) {
		img = flux_to ? &IconImg47::FluxTo() : &IconImg47::Flux();
	} else  {
		img = diss_conc ? &IconImg47::DissolvedConc() : &IconImg47::Property();
	}
	
	nodes.Create(var_id, name);
	int new_node = tree.Add(at, *img, nodes.Top());
	if(!var->store_series)
		tree.SetNode(new_node, tree.GetNode(new_node).CanSelect(false));
}

void
SeriesSelecter::build(Model_Application *app) {
	std::unordered_map<Var_Location, int, Var_Location_Hash> loc_to_node;
	
	bool show_flux = option_show_fluxes.Get();
	
#if CATCH_ERRORS
	try {
#endif
		if(type == Var_Id::Type::state_var) {
			for(int n_comp = 1; n_comp < max_var_loc_components; ++n_comp) {
				for(int pass = 0; pass < 4; ++pass) {
					for(Var_Id var_id : app->vars.all_state_vars())
						add_series_node(parent, var_tree, nodes, app, var_id, 0, loc_to_node, pass, show_flux, false, n_comp);
				}
			}
			for(Var_Id var_id : app->vars.all_state_vars()) {
				auto var = app->vars[var_id];
				if(var->type != State_Var::Type::step_resolution) continue;
				auto var2 = as<State_Var::Type::step_resolution>(var);
				std::string &name = app->model->solvers[var2->solver_id]->name;
				nodes.Create(var_id, name);
				Image *img = &IconImg47::Step();
				int new_node = var_tree.Add(0, *img, nodes.Top());
				//tree.SetNode(new_node, tree.GetNode(new_node).CanSelect(false));
				//log_print("Trallala: ", name, "\n");
			}
			
			loc_to_node.clear();
			
			for(int pass = 0; pass < 4; ++pass) {
				for(Var_Id var_id : app->vars.all_state_vars())
					add_series_node(parent, quant_tree, nodes, app, var_id, 0, loc_to_node, pass, show_flux, true);
			}
			
			for(auto select_id : parent->data_set->quick_selects) {
				auto quick_select = parent->data_set->quick_selects[select_id];
				
				nodes.Create(select_id, quick_select->name.c_str());
				Image *img = &IconImg47::Compartment();
				int nodeid = quick_tree.Add(0, *img, nodes.Top());
				quick_tree.SetNode(nodeid, quick_tree.GetNode(nodeid).CanSelect(false));
				
				for(int idx = 0; idx < quick_select->selects.size(); ++idx) {
					auto &select = quick_select->selects[idx];
					nodes.Create(select_id, select.name, idx);
					img = &IconImg47::Property();
					quick_tree.Add(nodeid, *img, nodes.Top());
				}
			}
			
			
		} else {

			int input_id = var_tree.Add(0, Null, "Model inputs", false);
			int additional_id = var_tree.Add(0, Null, "Additional series", false);
			
			var_tree.SetNode(input_id, var_tree.GetNode(input_id).CanSelect(false));
			var_tree.SetNode(additional_id, var_tree.GetNode(additional_id).CanSelect(false));
			
			for(Var_Id var_id : app->vars.all_series())
				add_series_node(parent, var_tree, nodes, app, var_id, input_id, loc_to_node, 2);
			
			for(Var_Id var_id : app->vars.all_additional_series())
				add_series_node(parent, var_tree, nodes, app, var_id, additional_id, loc_to_node, 2);
		}
#if CATCH_ERRORS
	} catch (int) {}
#endif
	parent->log_warnings_and_errors();
	
	var_tree.OpenDeep(0, true);
	quant_tree.OpenDeep(0, true);
	quick_tree.OpenDeep(0, true);
}