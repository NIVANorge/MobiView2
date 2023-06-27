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
	
	buf << "[3 The model [* " << model_name << "] contains the following modules:&&";
	
	MarkdownConverter mdc;
	
	// TODO: What to do here if there are multiple specializations of the same module template?
	
	for(auto module_id : parent->app->model->modules) {
		auto mod = parent->app->model->modules[module_id];
		auto temp = parent->app->model->module_templates[mod->template_id];
		
		buf += Format("[* %s (v. %d.%d.%d)]&", mod->full_name.data(), temp->version.major, temp->version.minor, temp->version.revision);
		if(!temp->doc_string.empty())
			buf += mdc.ToQtf(temp->doc_string.data());
		else
			buf +=  mdc.ToQtf("(no description provided by model creator)"); // Just to get the same formatting.
	}
	buf += "]";
	
	info_box.SetQTF(buf);
}