#include "Statistics.h"
#include "MobiView2.h"

using namespace Upp;

void
display_stat(const char *name, double val, String &display, bool &tracked_first, int precision) {
	if(!tracked_first) display << "::@W ";
	tracked_first = false;
	display << name << "::@W " << FormatDouble(val, precision);
}

void
display_stat(const char *name, int type, double val_old, double val_new, bool display_diff, String &display, bool &tracked_first, int precision) {
	if(!tracked_first) display << "::@W ";
	tracked_first = false;
	display << name << "::@W " << FormatDouble(val_new, precision);
	if(display_diff && val_old != val_new && std::isfinite(val_old) && std::isfinite(val_new)) {
		if(    (type==1 && val_new > val_old)
			|| (type==0 && val_new < val_old)
			|| (type==-1 && std::abs(val_new) < std::abs(val_old)))
			display << "::@G ";
		else
			display << "::@R ";
		if(val_new > val_old) display << "+";
		display << FormatDouble(val_new - val_old, precision);
	} else
		display << ":: ";
	display << "\n";
}

void
display_statistics(MyRichView *plot_info, Display_Stat_Settings *settings, Time_Series_Stats *stats, Color &color, String &label, String &unit) {
	
	int precision = settings->decimal_precision;
	
	String display = label + " [" + unit + "]:&";
	display.Replace("[", "`[");
	display.Replace("]", "`]");
	display.Replace("_", "`_");
	
	display = Format("[*@(%d.%d.%d) %s]", color.GetR(), color.GetG(), color.GetB(), display);
	
	bool tracked_first = true;
	display << "{{1:1FWGW ";
	#define SET_STATISTIC(handle, name) \
		if(settings->display_##handle) display_stat(name, stats->handle, display, tracked_first, precision);
	#include "support/stat_types.incl"
	#undef SET_STATISTIC

	
	display_stat("data points", stats->data_points, display, tracked_first, precision);
	if(settings->display_initial)
		display_stat("initial value", stats->initial_value, display, tracked_first, precision);
	display << "}}&";
	
	plot_info->append(display);
	plot_info->ScrollEnd();
}

void
display_residual_stats(MyRichView *plot_info, Display_Stat_Settings *settings, Residual_Stats *stats, Residual_Stats *cached_stats, bool display_diff, String &title) {
	int precision = settings->decimal_precision;
	
	String display = title;
	display.Replace("[", "`[");
	display.Replace("]", "`]");
	display.Replace("_", "`_");
	display = Format("[* %s]", display);
	display << "{{2:1:1FWGW ";
	
	bool tracked_first = true;
	#define SET_RESIDUAL(handle, name, type) \
		if(settings->display_##handle) display_stat(name, type, cached_stats->handle, stats->handle, display_diff, display, tracked_first, precision);
	#include "support/residual_types.incl"
	#undef SET_RESIDUAL
	
	display_stat("data points", stats->data_points, display, tracked_first, precision);
	display << ":: }}&";
	
	plot_info->append(display);
	plot_info->ScrollEnd();
}


EditStatSettings::EditStatSettings(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this, "Edit statistics settings");
	
	ok_button.WhenAction = THISBACK(save_and_close);
	
	reset_default_button.WhenAction << [this]() {
		percentiles_edit.SetText("2.5, 5, 15, 25, 50, 75, 85, 95, 97.5");
	};
	
	percentiles_edit.WhenEnter = THISBACK(save_and_close);
}

void EditStatSettings::save_and_close() {
	#define SET_STATISTIC(handle, name) \
		display_settings.display_##handle = (bool)display_##handle.Get();
	#include "support/stat_types.incl"
	#undef SET_STATISTIC
	
	#define SET_RESIDUAL(handle, name, type) \
		display_settings.display_##handle = (bool)display_##handle.Get();
	#include "support/residual_types.incl"
	#undef SET_RESIDUAL
	
	std::vector<double> percentiles;
	String perc_str = percentiles_edit.GetText().ToString();
	bool success = parse_percent_list(perc_str, percentiles);
	
	display_settings.decimal_precision = precision_edit.GetData();
	settings.eckhardt_filter_param = eckhardt_edit.GetData();
	display_settings.display_initial = display_initial.GetData();
	
	if(success && !percentiles.empty()) {
		std::sort(percentiles.begin(), percentiles.end());
		settings.percentiles = percentiles;
		Close();
		parent->plot_rebuild(); // To reflect the new settings!
	}
	else
		PromptOK("The percentiles has to be a comma-separated list of numeric values between 0 and 100, containing at least one value.");
}

void EditStatSettings::load_interface() {
	#define SET_STATISTIC(handle, name) \
		display_##handle.Set((int)display_settings.display_##handle);
	#include "support/stat_types.incl"
	#undef SET_STATISTIC
	
	#define SET_RESIDUAL(handle, name, type) \
		display_##handle.Set((int)display_settings.display_##handle);
	#include "support/residual_types.incl"
	#undef SET_RESIDUAL
	
	String q_str;
	int idx = 0;
	for(double q : settings.percentiles){
		q_str << Format("%g", q);
		if(idx++ != settings.percentiles.size()-1) q_str << ", ";
	}
	percentiles_edit.SetText(q_str);
	precision_edit.SetData(display_settings.decimal_precision);
	eckhardt_edit.SetData(settings.eckhardt_filter_param);
	display_initial.SetData(display_settings.display_initial);
}