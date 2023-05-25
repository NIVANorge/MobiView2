#include "ModelGraph.h"
#include "MobiView2.h"

using namespace Upp;

#include "support/graph_visualisation.h"
#include <graphviz/gvc.h>

// TODO: This is a hack. For some reason, we don't get this with the libs we use. Should
// rebuild them?
Agdesc_t 	Agdirected = { 1, 0, 0, 1 };


void
ModelGraph::rebuild_image(Model_Application *app) {
	
	if(!app) {
		image = Image();
		return;
	}
#ifndef _DEBUG     // NOTE: Currently we don't have debug versions of the graphviz libraries
	
	GVC_t *gvc = gvContext();

	Agraph_t *g = agopen("Model graph", Agdirected, 0);
	
	build_flux_graph(app, g, show_properties, show_flux_labels, show_short_names);
	
	gvLayout(gvc, g, "dot");
	
	char *data = nullptr;
	unsigned int len = 0;
	int res = gvRenderData(gvc, g, "bmp", &data, &len);
	
	if(res >= 0) { // TODO: Correct error handling
		image = StreamRaster::LoadAny(MemReadStream(data, len));
		gvFreeRenderData(data);
	} else {
		fatal_error(Mobius_Error::internal, "Graphviz error when rendering model graph.");
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
	
	//if(!has_image) rebuild_image();	
    
    w.DrawImage(0, 0, image);
#endif
}