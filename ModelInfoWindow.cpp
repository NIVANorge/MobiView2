#include "ModelInfoWindow.h"

#include "MobiView2.h"

#include <plugin/md/Markdown.h>

#define IMAGECLASS LogoImg
#define IMAGEFILE <MobiView/logos.iml>
#include <Draw/iml.h>


using namespace Upp;

ModelInfoWindow::ModelInfoWindow(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "Model information");
	Sizeable().Zoomable();
	info_box.is_log_window = false;
}

void ModelInfoWindow::refresh_text() {
	info_box.Clear();
	
	if(!parent->model_is_loaded()) {
		info_box.SetQTF("[3 No model is loaded.]");
		return;
	}
	
	String buf;
	String model_name = parent->model->model_name.data();
	if(model_name.StartsWith("INCA"))
		buf << "@@iml:1512*768`LogoImg:INCALogo`&&";
	else if(model_name.StartsWith("Simply"))
		buf << "@@iml:2236*982`LogoImg:SimplyLogo`&&";
	else if(model_name.StartsWith("MAGIC"))
		buf << "@@iml:2476*1328`LogoImg:MAGICLogo`&&";
	
	buf << "[3 The model [* " << model_name << "] contains the following modules:\n";
	
	MarkdownConverter mdc;
	
	for(auto module_id : parent->app->model->modules) {
		auto mod = parent->app->model->modules[module_id];
		if(!mod->has_been_processed) continue;
		
		buf += Format("\n[* %s (v. %d.%d.%d)]\n", mod->name.data(), mod->version.major, mod->version.minor, mod->version.revision);
		if(!mod->doc_string.empty())
			buf += mdc.ToQtf(mod->doc_string.data());
		else
			buf +=  "(no description provided by model creator)";
		buf += "\n";
	}
	buf += "]";
	buf.Replace("\n", "&");
	
	info_box.SetQTF(buf);
}