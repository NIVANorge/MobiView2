#ifndef _MobiView2_Util_h_
#define _MobiView2_Util_h_

/*
inline Upp::String
str(const String_View &str) {
	return Upp::String(str.data, str.data+str.count);
}
*/

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

template<typename Handle_T>
inline Upp::String
make_index_string(Storage_Structure<Handle_T> *structure, std::vector<Index_T> indexes, Handle_T handle) {

	const std::vector<Entity_Id> &index_sets = structure->get_index_sets(handle);
	if(index_sets.empty()) return "";
	
	Upp::String result = "[";
	int idx = 0;
	for(const Entity_Id &index_set : index_sets) {
		if(idx++ != 0) result << " ";
		ASSERT(indexes[index_set.id].index_set == index_set);
		result << "\"" << structure->parent->get_index_name(indexes[index_set.id]).data() << "\"";
	}
	result << "]";
	return result;
}

inline Upp::String
make_parameter_index_string(Storage_Structure<Entity_Id> *structure, Indexed_Parameter *par) {
	
	const std::vector<Entity_Id> &index_sets = structure->get_index_sets(par->id);
	if(index_sets.empty()) return "";
	
	Upp::String result = "[";
	int idx = 0;
	bool once = false;
	for(const Entity_Id &index_set : index_sets) {
		if(idx++ != 0) result << " ";
		//ASSERT(par->indexes[index_set.id].index_set == index_set);
		
		auto index = par->indexes[index_set.id];
		if(is_valid(par->mat_col)) {
			if(once)
				index = par->mat_col;
			once = true;
		}
		
		if(par->locks[idx])
			result << "locked(\"" << structure->parent->model->index_sets[index_set]->name << "\")";
		else
			result << "\"" << structure->parent->get_index_name(index) << "\"";
	}
	result << "]";
	return result;
}

#endif
