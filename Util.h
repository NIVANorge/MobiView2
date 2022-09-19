#ifndef _MobiView2_Util_h_
#define _MobiView2_Util_h_

inline Upp::String
str(const String_View &str) {
	return Upp::String(str.data, str.data+str.count);
}

inline Upp::Time
convert_time(const Date_Time &dt) {
	s32 year, month, day, hour, minute, second;
	dt.year_month_day(&year, &month, &day);
	dt.hour_minute_second(&hour, &minute, &second);
	return Upp::Time(year, month, day, hour, minute, second);
}

inline Date_Time
convert_time(const Upp::Time &tm) {
	Date_Time result(tm.year, tm.month, tm.day);
	result.add_timestamp(tm.hour, tm.minute, tm.second);
	return result;
}

inline bool
parse_percent_list(String &list_str, std::vector<double> &result) {
	bool success = true;
	
	Vector<String> split = Split(list_str, ',');
	for(String &str : split) {
		double val = StrDbl(str);
		if(!IsNull(val) && val >= 0.0 && val <= 100.0)
			result.push_back(val);
		else
			success = false;
	}
	return success;
}

#endif