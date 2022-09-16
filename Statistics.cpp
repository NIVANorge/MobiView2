#include "Statistics.h"

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
	if(settings->display_initial_value)
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