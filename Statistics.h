#ifndef _MobiView2_Statistics_h_
#define _MobiView2_Statistics_h_

#include "support/statistics.h"
#include "MyRichView.h"

struct Display_Stat_Settings {
	#define SET_STATISTIC(handle, name) bool display_##handle = true;
	#include "support/stat_types.incl"
	#undef SET_STATISTIC
	
	#define SET_RESIDUAL(handle, name, type) bool display_##handle = true;
	#include "support/residual_types.incl"
	#undef SET_RESIDUAL
	
	bool display_initial_value;
	int decimal_precision = 5;
};

void display_statistics(MyRichView *plot_info, Display_Stat_Settings *settings, Time_Series_Stats *stats,
	Upp::Color &color, Upp::String &label, Upp::String &unit);
	
void display_residual_stats(MyRichView *plot_info, Display_Stat_Settings *settings, Residual_Stats *stats, Residual_Stats *cached_stats, bool display_diff, Upp::String &title);
	
	
class EditStatSettings {
	// TODO
public :
	Statistics_Settings   settings;
	Display_Stat_Settings display_settings;
	
};

#endif
