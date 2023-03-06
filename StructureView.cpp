#include "MobiView2.h"


StructureViewWindow::StructureViewWindow(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "MobiView2 model structure debug information");
	Sizeable().Zoomable();
	
	batch_structure.SetEditable(false);
	view_tabs.Add(batch_structure.SizePos(), "Batch structure");
	
	function_tree.SetEditable(false);
	view_tabs.Add(function_tree.SizePos(), "Pseudocode");
	
	llvm_ir.SetEditable(false);
	view_tabs.Add(llvm_ir.SizePos(), "LLVM IR");
}

void
StructureViewWindow::refresh_text() {
	if(!parent->app) return;
	
	batch_structure.Clear();
	batch_structure.Set(String(parent->app->batch_structure.data()));

	function_tree.Clear();
	function_tree.Set(String(parent->app->batch_code.data()));
	
	llvm_ir.Clear();
	llvm_ir.Set(String(parent->app->llvm_ir.data()));
}