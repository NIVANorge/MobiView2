#include "ModelGraph.h"
#include "MobiView2.h"

using namespace Upp;

#include <graphviz/gvc.h>

ModelGraph::ModelGraph(MobiView2 *parent) : parent(parent) {
	Sizeable();
}

// TODO: This is a hack. For some reason, we don't get this with the libs we use. Should
// rebuild them?
Agdesc_t 	Agdirected = { 1, 0, 0, 1 };


Agnode_t *
make_empty(Agraph_t *g) {
	static int nodeid = 0;
	static char buf[128];
	
	sprintf(buf, "nonode_%d", nodeid++);
	Agnode_t *n = agnode(g, buf, 1);
	
	agsafeset(n, "label", "", "");
	agsafeset(n, "shape", "none", "");
	agsafeset(n, "width", "0", "");
	agsafeset(n, "height", "0", "");
	
	return n;
}

struct
Node_Data {
	Agraph_t *subgraph;
	Agnode_t *node;
	int clusterid;
	std::vector<Entity_Id> ids;
};

void
add_quantity_node(Agraph_t *g, std::unordered_map<Var_Location, Node_Data, Var_Location_Hash> &nodes, Model_Application *app, Var_Location &loc) {
	static int clusterid = 0;
	static int nodeid = 0;
	static char buf[128];
	
	auto find = nodes.find(loc);
	if(find != nodes.end()) return;
	
	auto id = loc.last();
	auto comp = app->model->components[id];
	//if(comp->decl_type == Decl_Type::property) return;
	
	Var_Location above = loc;
	above.n_components--;
	Agraph_t *g_above;
	if(loc.n_components == 1)
		g_above = g;
	else {
		auto find = nodes.find(above);
		if(find == nodes.end())
			add_quantity_node(g, nodes, app, above);
		g_above = nodes[above].subgraph;
	}
	
	if(comp->decl_type == Decl_Type::quantity || comp->decl_type == Decl_Type::compartment) {
		sprintf(buf, "cluster_%d", clusterid);
		
		Agraph_t *sub = agsubg(g_above, buf, 1);
		
		sprintf(buf, "node_%d", nodeid++);
		
		//Agnode_t *n = agnode(sub, buf, 1);
		Agnode_t *n = make_empty(sub);
		nodes[loc] = Node_Data {sub, n, clusterid++, {}};
		//agsafeset(n, "shape", "plaintext", "");
		//agsafeset(n, "label", name.data(), "");
		//agsafeset(sub, "label", comp->name.data(), "");
		
		agsafeset(sub, "style", "filled", "");
		if(loc.n_components == 1)
			agsafeset(sub, "fillcolor", "lightgrey", "");
		else
			agsafeset(sub, "fillcolor", "white", "");
		
		nodes[loc].ids.push_back(id);
	} else if(loc.n_components > 1)
		nodes[above].ids.push_back(id);
}

Agnode_t *
add_connection_node(Agraph_t *g, std::vector<Agnode_t *> &connection_nodes, Entity_Id conn_id, Model_Application *app) {
	if(connection_nodes[conn_id.id]) return connection_nodes[conn_id.id];
	
	static int nodeid = 0;
	static char buf[128];
	
	sprintf(buf, "connode_%d", nodeid++);
	Agnode_t *n = agnode(g, buf, 1);
	agsafeset(n, "label", (char *)app->model->connections[conn_id]->name.data(), "");
	
	connection_nodes[conn_id.id] = n;
	return n;
}

void
add_flux_edge(Agraph_t *g, std::unordered_map<Var_Location, Node_Data, Var_Location_Hash> &nodes, 
	std::vector<Agnode_t *> &connection_nodes, Model_Application *app, State_Var *var, bool show_flux_labels) {
		
	static int edgeid = 0;
	static char buf[128];
	
	sprintf(buf, "edge_%d", edgeid++);
	std::string *name = nullptr;
	
	Var_Location *loc1 = nullptr;
	Var_Location *loc2 = nullptr;
	Entity_Id conn_id = invalid_entity_id;
	
	bool is_agg = false;
	if(var->type == State_Var::Type::declared) {
		if(var->flags & State_Var::Flags::has_aggregate) return;
		if(is_located(var->loc1)) loc1 = &var->loc1;
		if(is_located(var->loc2)) loc2 = &var->loc2;
		conn_id = var->loc2.connection_id;
		name = &var->name;
	} else if (var->type == State_Var::Type::regular_aggregate) {
		auto var2 = as<State_Var::Type::regular_aggregate>(var);
		auto agg_var = app->vars[var2->agg_of];
		if(agg_var->type != State_Var::Type::declared) return;
		loc1 = &agg_var->loc1;
		loc2 = &var->loc2;
		name = &agg_var->name;
		is_agg = true;
	} else
		return;
	
	Agnode_t *n1, *n2;
	if(loc1)
		n1 = nodes[*loc1].node;
	else
		n1 = make_empty(g);
	
	if(loc2)
		n2 = nodes[*loc2].node;
	else if(is_valid(conn_id))
		n2 = add_connection_node(g, connection_nodes, conn_id, app);
	else
		n2 = make_empty(g);
	
	Agedge_t *e = agedge(g, n1, n2, buf, 1);
	if(show_flux_labels) {
		agsafeset(e, "label", (char *)name->data(), "");
		agsafeset(e, "fontsize", "12", "");
	}
	
	if(loc1) {
		sprintf(buf, "cluster_%d", nodes[*loc1].clusterid);
		agsafeset(e, "ltail", buf, "");
	}
	if(loc2) {
		sprintf(buf, "cluster_%d", nodes[*loc2].clusterid);
		agsafeset(e, "lhead", buf, "");
	}
	if(is_agg)
		agsafeset(e, "arrowhead", "open", "");
}

void
ModelGraph::rebuild_image() {
	
#ifndef _DEBUG     // NOTE: Currently we don't have debug versions of the graphviz libraries
	if(!parent->model_is_loaded()) return;
	//if(has_image) return;
	has_image = true;
	GVC_t *gvc = gvContext();

	auto app = parent->app;

	// Create a simple digraph
	Agraph_t *g = agopen("Model graph", Agdirected, 0);
	
	
	
	bool show_properties = false;
	bool show_flux_labels = true;
	
	agsafeset(g, "compound", "true", "");
	
	std::unordered_map<Var_Location, Node_Data, Var_Location_Hash> nodes;
	std::vector<Agnode_t *> connection_nodes(app->model->connections.count(), nullptr);
	
	for(auto var_id : app->vars.all_state_vars()) {
		auto var = app->vars[var_id];
		if(var->type != State_Var::Type::declared) continue;
		auto var2 = as<State_Var::Type::declared>(var);
		if(var2->decl_type != Decl_Type::quantity && var2->decl_type != Decl_Type::property) continue;
		add_quantity_node(g, nodes, app, var->loc1);
	}
	
	for(auto &pair : nodes) {
		
		auto &data = pair.second;
		if(show_properties) {
			std::stringstream ss;
			ss << "<table cellborder=\"0\" border=\"0\">";
			int row = 0;
			for(auto id : data.ids) {
				ss << "<tr><td align=\"left\">";
				if(row == 0) ss << "<b>";
				else ss << "<font point-size=\"8\">";
				ss << app->model->components[id]->name;
				if(row == 0) ss << "</b>";
				else ss << "</font>";
				ss << "</td></tr>";
				++row;
			}
			ss << "</table>";
			auto str = ss.str();
			agsafeset(data.subgraph, "label", agstrdup_html(g, str.data()), "");
		} else
			agsafeset(data.subgraph, "label", (char *)app->model->components[data.ids[0]]->name.data(), "");
	}
	
	for(auto var_id : app->vars.all_fluxes()) {
		auto var = app->vars[var_id];
		//if(var->type != State_Var::Type::declared) continue;
		add_flux_edge(g, nodes, connection_nodes, app, var, show_flux_labels);
	}
	// Use the directed graph layout engine
	gvLayout(gvc, g, "dot");
	
	char *data = nullptr;
	unsigned int len = 0;
	int res = gvRenderData(gvc, g, "bmp", &data, &len);
	
	if(res >= 0) { // TODO: Correct error handling
		image = StreamRaster::LoadAny(MemReadStream(data, len));
		gvFreeRenderData(data);
	} else {
		parent->log("Graphviz error when rendering model graph.", true);
	}
	
	gvFreeLayout(gvc, g);

	agclose(g);
	
	Refresh();
#endif
}

void
ModelGraph::Paint(Draw& w) {
	w.DrawRect(GetSize(), White());
#ifndef _DEBUG
	
	if(!has_image) rebuild_image();	
    
    w.DrawImage(10, 10, image);
#endif
}