#include "Statistics.h"

using namespace Upp;

void display_stat(String name, double val, String &display, bool &tracked_first, int precision) {
	if(!tracked_first) display << "::@W ";
	tracked_first = false;
	display << name << "::@W " << FormatDouble(val, precision);
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