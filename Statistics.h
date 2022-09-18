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
	
	bool display_initial = false;
	int decimal_precision = 5;
};

void display_statistics(MyRichView *plot_info, Display_Stat_Settings *settings, Time_Series_Stats *stats,
	Upp::Color &color, Upp::String &label, Upp::String &unit);
	
void display_residual_stats(MyRichView *plot_info, Display_Stat_Settings *settings, Residual_Stats *stats, Residual_Stats *cached_stats, bool display_diff, Upp::String &title);

#define LAYOUTFILE <MobiView2/StatSettings.lay>
#include <CtrlCore/lay.h>

class MobiView2;

class EditStatSettings : public WithEditStatSettingsLayout<Upp::TopWindow> {
public :
	typedef EditStatSettings CLASSNAME;
	
	EditStatSettings(MobiView2 *parent);
	
	Statistics_Settings   settings;
	Display_Stat_Settings display_settings;
	MobiView2 *parent;
	
	void save_and_close();
	void load_interface();
	void read_from_json(Upp::Value &settings_json);
	void write_to_json(Upp::Json &settings_json);
};

#endif
