#include "ParameterCtrl.h"
#include "MobiView2.h"

using namespace Upp;

#define IMAGECLASS IconImg8
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

ParameterCtrl::ParameterCtrl(MobiView2 *parent) : parent(parent), map_edit(parent) {
	CtrlLayout(*this);
	
	parameter_view.AddColumn("Name");
	parameter_view.AddColumn("Value");
	parameter_view.AddColumn("Min");
	parameter_view.AddColumn("Max");
	parameter_view.AddColumn("Unit");
	parameter_view.AddColumn("Description");
	
	parameter_view.ColumnWidths("30 14 8 8 12 36");
	
	// TODO: This doesn't fire when only the col and not the row is changed, but there is no
	// easy solution except changing parameter_view to be a GridCtrl, which would mean we have
	// to learn that API and rewrite everything in this class :(
	parameter_view.WhenSel << [this]() {
		focus_row = -1;
		focus_col = -1;
		int row = parameter_view.GetCursor();
		if(row < 0 || row >= listed_pars.size())
			return;
		focus_row = row;
		
		int col_count = parameter_controls[row].size();
		if(col_count == 1)
			focus_col = 0;
		else {
			for(int col = 0; col < col_count; ++col) {
				if(parameter_controls[row][col].HasFocus()) {
					focus_col = col;
					break;
				}
			}
		}
	};
}

void ParameterCtrl::clean() {
	parameter_view.Clear();
	
	index_set_names.Clear();
	index_lists.Clear();
	index_locks.Clear();
	index_expands.Clear();

	index_sets.clear();
	
	par_group_id = invalid_entity_id;
	expanded_row = invalid_entity_id;
	expanded_col = invalid_entity_id;
	
	focus_row = -1;
	focus_col = -1;
	
	map_edit.clean();
}


void ParameterCtrl::build_index_set_selecters(Model_Application *app) {
	if(!app) return;
	auto model = app->model;
	
	Indexes indexes(model);
	for(auto index_set_id : model->index_sets) {
		auto index_set = model->index_sets[index_set_id];
		
		auto &index_set_name = index_set_names.Create<Label>();
		index_set_name.SetText(index_set->name.data());
		index_set_name.Hide();
		
		auto &lock = index_locks.Create<ButtonOption>();
		lock.Hide();
		lock.SetImage(IconImg8::LockOpen(), IconImg8::Lock());
		
		auto &expand = index_expands.Create<ButtonOption>();
		expand.Hide();
		expand.SetImage(IconImg8::Add(), IconImg8::Remove());
		expand.WhenAction << [this, index_set_id](){ expand_index_set_clicked(index_set_id); };
		
		auto &list = index_lists.Create<Upp::DropList>();
		// If the list is not dependent on other factors, just generate it once and for all
		if(!is_valid(index_set->sub_indexed_to)) {
			auto count = app->index_data.get_index_count(indexes, index_set_id);
			for(Index_T index = {index_set_id, 0}; index < count; ++index) {
				indexes.set_index(index, true);
				list.Add(app->index_data.get_index_name(indexes, index));
			}
			list.GoBegin();
		}
		
		list.Hide();
		list.WhenAction << [this, index_set_id](){ list_change(index_set_id); };
		
		Add(index_set_name.TopPosZ(4, 19));
		Add(list.TopPosZ(24, 19));
		Add(lock.TopPosZ(48, 18));
		Add(expand.TopPosZ(48, 18));
	}
}

void ParameterCtrl::par_group_change() {
	
	for(int idx = 0; idx < index_set_names.size(); ++idx) {
		index_set_names[idx].Hide();
		index_lists[idx].Hide();
		index_locks[idx].Hide();
		index_expands[idx].Hide();
	}
	
	focus_row = -1;
	focus_col = -1;
	
	par_group_id = invalid_entity_id;
	
	auto app = parent->app;
	auto model = app->model;
	
	if(!parent->model_is_loaded()) return; // Should not be possible, but safety stopgap.
	
	par_group_id = parent->get_selected_par_group();
	if(!is_valid(par_group_id))
		return;
	
	auto par_range = parent->model->get_scope(par_group_id)->by_type<Reg_Type::parameter>();
	s64  par_count = par_range.size();
	
	if(!par_count) return;
	
	index_sets = parent->app->parameter_structure.get_index_sets(*par_range.begin());
	
	// TODO: There may be a problem if two sub-index sets of the same parent are selected and
	// we now select another group where the parent is also present.
	for(int idx = 0; idx < index_sets.size(); ++idx) {
		index_expands[idx].Enable();
		
		for(int idx2 = idx+1; idx2 < index_sets.size(); ++idx2) {
			if(!index_expands[index_sets[idx2].id].GetData()) {
				if(app->index_data.can_be_sub_indexed_to(index_sets[idx], index_sets[idx2])) {
					index_expands[idx].Disable();
				}
			}
		}
	}
	
	if(index_sets.size() > MAX_INDEX_SETS)
		parent->log(Format("Warning: displaying more than %d index sets at the same time may make the display strange.", MAX_INDEX_SETS));
	
	Indexes indexes(model);
	int pos = 0;
	for(auto id : index_sets) {
		auto index_set = model->index_sets[id];
		
		index_set_names[id.id].LeftPosZ(4 + 108*pos, 100);
		index_set_names[id.id].Show();
		
		auto &list = index_lists[id.id];
		list.LeftPosZ(4 + 108*pos, 104);
		
		int count = app->index_data.get_index_count(indexes, id).index;
		
		int existing_sel = list.GetIndex();
		if(existing_sel < 0) existing_sel = 0;
		else if(existing_sel >= count) existing_sel = count-1; // TODO: Is this good behaviour?
		
		if(is_valid(index_set->sub_indexed_to)) {
			// NOTE: The list depends on the configuration of other indexes.
			list.Clear();
			for(Index_T index = {id, 0}; index.index < count; ++index) {
				indexes.set_index(index, true);
				list.Add(app->index_data.get_index_name(indexes, index));
			}
			if(list.GetCount())
				list.SetIndex(std::max(0, existing_sel));

		}
		
		Index_T index = Index_T { id, existing_sel };
		indexes.set_index(index, true);
		
		list.Show();
		
		index_locks[id.id].LeftPosZ(4 + 108*pos, 18);
		index_locks[id.id].Show();
		
		index_expands[id.id].LeftPosZ(24 + 108*pos, 18);
		index_expands[id.id].Show();
		
		++pos;
	}
	
	Refresh();
	
	refresh_parameter_view(false);
}

void ParameterCtrl::list_change(Entity_Id id) {
	
	auto app = parent->app;
	
	Indexes indexes(parent->model);
	for(auto id2 : index_sets) {
		
		// TODO: Some of this could be factored out (see exact same code above).
		
		auto &list = index_lists[id2.id];
		
		int count = app->index_data.get_index_count(indexes, id2).index;
		int existing_sel = list.GetIndex();
		if(existing_sel < 0) existing_sel = 0;
		else if(existing_sel >= count) existing_sel = count-1;
		if(!list.GetCount()) existing_sel = -1; 
		
		auto index_set = parent->model->index_sets[id2];
		if(id != id2 && is_valid(index_set->sub_indexed_to) && app->index_data.can_be_sub_indexed_to(id, index_set->sub_indexed_to)) {
			list.Clear();
			for(Index_T index = {id, 0}; index.index < count; ++index) {
				indexes.set_index(index, true);
				list.Add(app->index_data.get_index_name(indexes, index));
			}
			if(list.GetCount())
				list.SetIndex(std::max(0, existing_sel));
		}
		
		Index_T index = { id2, existing_sel };
		indexes.set_index(index);
	}
	
	refresh_parameter_view(false);
}

void
ParameterCtrl::switch_expanded(Entity_Id id, bool on, bool force) {
	
	if(!is_valid(id)) return;
	
	if(on) {
		index_locks[id.id].Disable();
		index_lists[id.id].Disable();
		
		if(is_valid(expanded_row)) {
			switch_expanded(expanded_col, false, true);
			if(expanded_row < id)
				expanded_col = id;
			else {
				expanded_col = expanded_row;
				expanded_row = id;
			}
		} else {
			expanded_row = id;
		}
		
		if(force)
			index_expands[id.id].SetData(true);
	} else {
		index_locks[id.id].Enable();
		index_lists[id.id].Enable();
		
		if(expanded_row == id) {
			if(is_valid(expanded_col)) {
				expanded_row = expanded_col;
				expanded_col = invalid_entity_id;
			} else
				expanded_row = invalid_entity_id;
		} else if(expanded_col == id)
			expanded_col = invalid_entity_id;
		
		if(force)
			index_expands[id.id].SetData(false);
	}
	
}

void ParameterCtrl::expand_index_set_clicked(Entity_Id id) {
	
	bool is_on = index_expands[id.id].Get();
	
	// Special logic: If a sub-indexed index set is selected, it should select a parent
	// index set of it also if one is among the selectable.
	auto parent_id = invalid_entity_id;
	if(is_valid(parent->model->index_sets[id]->sub_indexed_to)) {
		for(auto id2 : index_sets) {
			if(id2 == id) break;
			if(parent->app->index_data.can_be_sub_indexed_to(id2, id)) {
				parent_id = id2;
				break;
			}
		}
	}
	if(is_valid(parent_id)) {
		if(is_on)
			index_expands[parent_id.id].Enable();
		else {
			switch_expanded(parent_id, false, true);
			index_expands[parent_id.id].Disable();
		}
	}

	switch_expanded(id, is_on);
		
	refresh_parameter_view(false);
}

void
ParameterCtrl::set_single_parameter(ValueMap &row_data, Indexed_Parameter &par_data, Id &value_column, int row, int col, bool show_additional, bool values_only, bool can_edit, bool map_form) {
	
	if(!values_only)
		listed_pars[row][col] = par_data;
	
	s64 offset = parent->app->parameter_structure.get_offset(par_data.id, par_data.indexes);		
	Parameter_Value val = *parent->app->data.parameters.get_value(offset);
	
	auto par = parent->model->parameters[par_data.id];
	
	if(map_form) {
		
		if(par->decl_type != Decl_Type::par_real) {
			fatal_error(Mobius_Error::internal, "Unsupported parameter type for map form");
		}
			
		if(!values_only) {
			parameter_controls[row].Create<Button>();
			auto &ctrl = parameter_controls[row][col];
			static_cast<Button &>(ctrl).SetImage(IconImg8::Edit());
			ctrl.WhenAction = [this, row, col]() {
				map_edit.select_par(listed_pars[row][col]);
				if(!map_edit.IsOpen())
					map_edit.Open();
			};
			parameter_view.SetCtrl(row, parameter_view.GetPos(value_column), ctrl);
		}
		
		static_cast<Button &>(parameter_controls[row][col]).SetLabel(FormatDouble(val.val_real));
		
		return;
	}
	
	
	if(par->decl_type == Decl_Type::par_real) {
		row_data.Set(value_column, val.val_real);
		if(show_additional) {
			row_data.Set("__min", par->min_val.val_real);
			row_data.Set("__max", par->max_val.val_real);
		}
		
		if(!values_only) {
			parameter_controls[row].Create<EditDoubleNotNull>();
			parameter_controls[row][col].WhenAction = [row, col, this]() {
				EditDoubleNotNull *value_field = (EditDoubleNotNull *)&parameter_controls[row][col];
				if(IsNull(*value_field)) return;
				Parameter_Value val;
				val.val_real = (double)*value_field;
				parameter_edit(listed_pars[row][col], parent->app, val);
			};
		}
	} else if(par->decl_type == Decl_Type::par_int) {
		row_data.Set(value_column, val.val_integer);
		if(show_additional) {
			row_data.Set("__min", par->min_val.val_integer);
			row_data.Set("__max", par->max_val.val_integer);
		}
		
		if(!values_only) {
			parameter_controls[row].Create<EditInt64NotNullSpin>();
			parameter_controls[row][col].WhenAction = [row, col, this]() {
				EditInt64NotNullSpin *value_field = (EditInt64NotNullSpin *)&parameter_controls[row][col];
				if(IsNull(*value_field)) return;
				Parameter_Value val;
				val.val_integer = (int64)*value_field;
				parameter_edit(listed_pars[row][col], parent->app, val);
			};
		}
	} else if(par->decl_type == Decl_Type::par_bool) {
		row_data.Set(value_column, (int)val.val_boolean);

		if(!values_only) {
			parameter_controls[row].Create<Option>();
			parameter_controls[row][col].WhenAction = [row, col, this]() {
				Option *value_field = (Option *)&parameter_controls[row][col];
				Parameter_Value val;
				val.val_boolean = (bool)value_field->Get();
				parameter_edit(listed_pars[row][col], parent->app, val);
			};
		}
	} else if(par->decl_type == Decl_Type::par_datetime) {
		Time tm = convert_time(val.val_datetime);
		row_data.Set(value_column, tm);
		
		if(!values_only) {
			parameter_controls[row].Create<EditTimeNotNull>();
			parameter_controls[row][col].WhenAction = [row, col, this]() {
				EditTimeNotNull *value_field = (EditTimeNotNull *)&parameter_controls[row][col];
				Time tm = value_field->GetData();
				if(IsNull(tm)) return;
				Parameter_Value val;
				val.val_datetime = convert_time(tm);
				parameter_edit(listed_pars[row][col], parent->app, val);
			};
		}
	} else if(par->decl_type == Decl_Type::par_enum) {
		row_data.Set(value_column, val.val_integer);
		
		if(!values_only) {
			parameter_controls[row].Create<DropList>();
			DropList *enum_list = (DropList *)&parameter_controls[row][col];
			
			int64 key = 0;
			for(auto &name : par->enum_values)
				enum_list->Add(key++, name.data());
			enum_list->GoBegin();
			
			parameter_controls[row][col].WhenAction = [row, col, this]() {
				DropList *value_field = (DropList *)&parameter_controls[row][col];
				Parameter_Value val;
				val.val_integer = (int64)value_field->GetKey(value_field->GetIndex());
				parameter_edit(listed_pars[row][col], parent->app, val);
			};
		}
	}
	
	if(!values_only) {
		auto &ctrl = parameter_controls[row][col];
		parameter_view.SetCtrl(row, parameter_view.GetPos(value_column), ctrl);
		if(!can_edit)
			ctrl.Disable();
	}
}


void
ParameterCtrl::refresh_parameter_view(bool values_only) {
	
	if(!values_only) {
		parameter_view.Clear();
		parameter_view.Reset();
		parameter_view.NoVertGrid();
		
		parameter_controls.Clear();
	}
	
	if(!parent->model_is_loaded()) return; // Should not be possible, but safety stopgap.
	if(!is_valid(par_group_id)) return;
	
	auto app = parent->app;
	auto model = app->model;
	
	
	Entity_Id exp_row = invalid_entity_id;
	Entity_Id exp_col = invalid_entity_id;
	bool special_subindexing = false;
	
	{
		bool row_is_valid = is_valid(expanded_row) && (std::find(index_sets.begin(), index_sets.end(), expanded_row) != index_sets.end());
		bool col_is_valid = is_valid(expanded_col) && (std::find(index_sets.begin(), index_sets.end(), expanded_col) != index_sets.end());
		
		special_subindexing = (row_is_valid && col_is_valid && app->index_data.can_be_sub_indexed_to(expanded_row, expanded_col));
		
		if(!special_subindexing) {
			if(!is_valid(expanded_row) || !index_expands[expanded_row.id].IsEnabled()) row_is_valid = false;
			if(!is_valid(expanded_col) || !index_expands[expanded_col.id].IsEnabled()) col_is_valid = false;
		}
		
		if(row_is_valid) {
			exp_row = expanded_row;
			if(col_is_valid)
				exp_col = expanded_col;
		} else if(col_is_valid)
			exp_row = expanded_col;

	}
	
	auto par_range = model->get_scope(par_group_id)->by_type<Reg_Type::parameter>();
	s64  par_count = par_range.size();
	
	bool empty = false;
	
	//NOTE: We don't store info about the parameter being locked now, since that has to be
	//overridden later anyway (the lock status when the look-up happens can have changed since the table was
	//constructed.
	Indexed_Parameter par_data(parent->model);	
	for(auto id : index_sets) {
		int sel = index_lists[id.id].GetIndex();
		if(sel < 0 || !index_lists[id.id].GetCount())
			empty = true;
		par_data.indexes.set_index(Index_T { id, sel });
	}
	
	
	if(!values_only) {
		parameter_view.AddColumn(Id("__name"), "Name");
		
		if(is_valid(exp_row)) {
			std::string name = model->index_sets[exp_row]->name;
			if(is_valid(exp_col))
				name += " \\ " + model->index_sets[exp_col]->name;
			parameter_view.AddColumn(Id("__index"), name.data());
		}
		
		parameter_view.HeaderObject().Proportional();
		parameter_view.AutoHideHorzSb(true);
		
		s32 col_max_count = 1;
		if(is_valid(exp_col)) {
			col_max_count = app->index_data.get_max_count(exp_col).index;
			
			if(col_max_count > 5) {
				parameter_view.HeaderObject().Absolute();
				parameter_view.AutoHideHorzSb(false);
			}
			
			if(special_subindexing) {
				for(int col = 0; col < col_max_count; ++col) {
					std::string name = std::to_string(col);
					parameter_view.AddColumn(Id(name.data()), name.data());
				}
			} else {
				//Note: This breaks if somebody calls one of the indexes e.g. "__name". Do we
				//need a better solution?
				for(Index_T col_idx = {exp_col, 0}; col_idx.index < col_max_count; ++col_idx) {
					auto &name = parent->app->index_data.get_index_name(par_data.indexes, col_idx);
					parameter_view.AddColumn(Id(name.data()), name.data());
				}
			}
			
		} else
			parameter_view.AddColumn(Id("__value"), "Value");
		
		parameter_view.AddColumn(Id("__min"), "Min");
		parameter_view.AddColumn(Id("__max"), "Max");
		parameter_view.AddColumn(Id("__unit"), "Unit");
		parameter_view.AddColumn(Id("__description"), "Description");
		
		if(is_valid(exp_col)) {
			//NOTE Hide the min, max, unit fields. We still have to add them since we use the
			//info stored in them some other places.
			int tab_base = 2 + col_max_count;
			parameter_view.HeaderObject().HideTab(tab_base + 0);
			parameter_view.HeaderObject().HideTab(tab_base + 1);
			parameter_view.HeaderObject().HideTab(tab_base + 2);
			parameter_view.HeaderObject().HideTab(tab_base + 3);
			
			std::stringstream ss;
			ss << "90 110";
			for(int idx = 0; idx < col_max_count; ++idx)
				ss << " 80";
			parameter_view.ColumnWidths(ss.str().data());
		} else {
			if(is_valid(exp_row))
				parameter_view.ColumnWidths("30 14 8 8 8 10 30");
			else
				parameter_view.ColumnWidths("30 14 8 8 12 36");
		}
	}
	
	if(empty) return;
	
	if(!values_only)
		listed_pars.clear();
	
	int par_idx = 0;
	
	Color row_colors[2] = {{255, 255, 255}, {240, 240, 255}};
	
	s32 row_count = 1;
	if(is_valid(exp_row))
		row_count = app->index_data.get_index_count(par_data.indexes, exp_row).index;
	
	if(!values_only)
		listed_pars.resize(par_count*row_count);
	
	int row = 0;
	for(Entity_Id par_id : par_range) {
		
		par_data.id = par_id;
		auto par = model->parameters[par_id];
		
		auto edit_form = app->get_parameter_edit_form(par_id);
		bool can_edit = (edit_form == Model_Application::Edit_Form::direct);
		bool map_form = (edit_form == Model_Application::Edit_Form::map);
		
		for(s32 idx_row = 0; idx_row < row_count; ++idx_row) {
		
			if(!values_only) {
				parameter_view.Add();
				
				parameter_controls.Create<Array<Ctrl>>();
			}
			
			ValueMap row_data;
			
			Index_T row_idx = invalid_index;
			if(is_valid(exp_row)) {
				row_idx = Index_T { exp_row, idx_row };
				par_data.indexes.set_index(row_idx, true);
				row_data.Set("__index", app->index_data.get_index_name(par_data.indexes, row_idx).data());
			}
			
			bool show_additional = (idx_row == 0);
			
			// For expanded row, only show name, description, etc for the first row of each parameter
			if(show_additional) {
				row_data.Set("__name", par->name.data());
				if(is_valid(par->unit)) {
					auto unit = model->units[par->unit];
					std::string unit_str = unit->data.to_utf8();
					row_data.Set("__unit", unit_str.data());
				}
				if(!par->description.empty()) row_data.Set("__description", par->description.data());
			}
			
			s32 col_count = 1;
			if(is_valid(exp_col))
				col_count = app->index_data.get_index_count(par_data.indexes, exp_col).index;
			
			if(!values_only)
				listed_pars[row].resize(col_count, Indexed_Parameter(model));
			
			for(int col = 0; col < col_count; ++col) {
				
				Id value_column = "__value";
				if(is_valid(exp_col)) {
					
					Index_T col_idx = Index_T { exp_col, col };
					par_data.indexes.set_index(col_idx, true);
					
					if(special_subindexing) {
						std::string name = std::to_string(col);
						value_column = Id(name.data());
					} else
						value_column = Id(app->index_data.get_index_name(par_data.indexes, col_idx).data());
				}
				
				set_single_parameter(row_data, par_data, value_column, row, col, show_additional, values_only, can_edit, map_form);

			}
			
			parameter_view.SetMap(row, row_data);
			// Alternating row colors for expanded indexes
			if(is_valid(exp_row)) {
				Color &row_color = row_colors[par_idx % 2];
				parameter_view.SetLineColor(row, row_color);
			}
			
			++row;
		}
		++par_idx;
	}
}

void
ParameterCtrl::parameter_edit(Indexed_Parameter par_data, Model_Application *app, Parameter_Value val) {
	
	if(!is_valid(par_data.id)) {
		parent->log("Internal error: Trying to set value of parameter that is flagged as invalid or virtual.", true);
		return;
	}
	
	// Fetch the lock status of the lock controls.
	set_locks(par_data);
	
	try {
		set_parameter_value(par_data, &app->data, val);
		changed_since_last_save = true;
	} catch(int) {}
	parent->log_warnings_and_errors();
	
	// TODO: Do we really want to do this here? Ideally it should be a part of Mobius2 itself
	// so that it also happens if done through other APIs.
	if(app->is_baked_parameter(par_data.id)) {
		// NOTE: We have to put it on the event queue instead of doing it immediately, because
		// otherwise we may end up deleting the parameter editor Ctrl inside one of its
		// callbacks, which can cause a crash.
		PostCallback([this]() {
			parent->reload(true);
			parent->log("The model was recompiled due to a change in a constant parameter."); // Is there a better name than "constant parameter"?
		});
	}
}

void
ParameterCtrl::set_locks(Indexed_Parameter &par) {
	
	for(int idx = 0; idx < par.locks.size(); ++idx)
		par.locks[idx] = (u8)(index_locks[idx].IsEnabled() && (bool)index_locks[idx].Get());
}

Indexed_Parameter
ParameterCtrl::get_selected_parameter() {
	
	Indexed_Parameter result(parent->model);
	result.id = invalid_entity_id;
	
	int row = parameter_view.GetCursor();
	if(focus_row < 0 || focus_col < 0) return std::move(result);
	
	// TODO: This is hacky... Do we want to switch to GridCtrl for parameter editing instead?
	
	result = listed_pars[focus_row][focus_col];
	set_locks(result);
	
	return std::move(result);
}

std::vector<Indexed_Parameter>
ParameterCtrl::get_all_parameters() {
	std::vector<Indexed_Parameter> result;
	
	for(auto &row : listed_pars)
		result.insert(result.end(), row.begin(), row.end());
	
	for(auto &par : result)
		set_locks(par);
	
	return std::move(result);
}

std::vector<Entity_Id>
ParameterCtrl::get_row_parameters() {
	std::vector<Entity_Id> result;
	
	for(int row = 0; row < listed_pars.size(); ++row) {
		if(listed_pars[row].empty())
			result.push_back(invalid_entity_id);
		else
			result.push_back(listed_pars[row][0].id);  // All parameters of a row are the same id.
	}
	
	return std::move(result);
}



ParameterMapCtrl::ParameterMapCtrl(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this);
	Sizeable().Zoomable();
	
	push_add.SetImage(IconImg8::Add());
	push_add.WhenAction = THISBACK(add_pushed);
	
	push_remove.SetImage(IconImg8::Remove());
	push_remove.WhenAction = THISBACK(remove_pushed);
	
	map_view.AddColumn("Position"); //TODO: Could call the index column after the index set.
	map_view.AddColumn("Value");
}

void
ParameterMapCtrl::clean(bool clean_par) {
	map_view.Clear();
	value_ctrls.Clear();
	pos_ctrls.Clear();
	data_index.clear();
	if(clean_par) {
		current_par = invalid_entity_id;
		current_par_app = invalid_entity_id;
	}
}

void
ParameterMapCtrl::select_par(Indexed_Parameter &par) {
	current_par_app = par.id;
	
	current_indexes.clear();
	// Map the indexes over to something that works for the data_set
	auto &index_sets = parent->app->parameter_structure.get_index_sets(par.id);
	for(auto set_id : index_sets) {
		auto index = par.indexes.get_index(parent->app->index_data, set_id);
		index.index_set = map_id(parent->model, parent->data_set, set_id);
		current_indexes.add_index(index);
	}
	
	current_par = map_id(parent->model, parent->data_set, current_par_app);
	refresh();
}

bool
match_initial(Indexes &a, Indexes &b) {
	if(!a.lookup_ordered || !b.lookup_ordered || a.indexes.size() != b.indexes.size())
		fatal_error(Mobius_Error::internal, "Unimplemented form for indexes using match_initial");

	for(int idxidx = 0; idxidx < (int)a.indexes.size()-1; ++idxidx) {
		if(a.indexes[idxidx] != b.indexes[idxidx]) return false;
	}
	return true;
}

void
ParameterMapCtrl::show_at_row(int row_idx, int data_idx, Parmap_Entry &entry) {
	
	map_view.Insert(row_idx, {entry.pos, entry.value} );
			
	// TODO: This must also work if it is not positional but index-based.
	pos_ctrls.Insert(row_idx);
	auto &posctrl = pos_ctrls[row_idx];
	map_view.SetCtrl(row_idx, 0, posctrl);
	posctrl.WhenAction << [this, row_idx]() {
		EditDoubleNotNull &ctrl = pos_ctrls[row_idx];
		Value val = ctrl.GetData();
		if(IsNull(val)) return;
		pos_update(row_idx, (double)val);
	};
	
	value_ctrls.Insert(row_idx);
	auto &ctrl = value_ctrls[row_idx];
	//ctrl.SetData(entry.value);
	map_view.SetCtrl(row_idx, 1, ctrl);
	ctrl.WhenAction << [this, row_idx]() {
		EditDoubleNotNull &ctrl = value_ctrls[row_idx];
		Value val = ctrl.GetData();
		if(IsNull(val)) return;
		value_update(row_idx, (double)val);
	};
	
	data_index.insert(data_index.begin()+row_idx, data_idx);
	
	map_view.SetLineColor(row_idx, Color(255, 255, 255));
}

void
ParameterMapCtrl::refresh() {
	clean(false);
	
	if(!parent->model_is_loaded()) return;
	if(!is_valid(current_par)) return;
	
	auto data_set = parent->data_set;

	auto pardata = data_set->parameters[current_par];
	
	if(!pardata->is_on_map_form) {
		PromptOK("Something went wrong, this parameter is not on map form!");
		return;
	}
	
	auto &index_sets = parent->app->parameter_structure.get_index_sets(current_par_app);
	
	int data_idx = 0;
	int row_idx = 0;
	for(auto &entry : pardata->parmap_data) {
		if(match_initial(entry.indexes, current_indexes)) {
			
			show_at_row(row_idx, data_idx, entry);
			
			++row_idx;
		}
			
		++data_idx;
	}
}

void
ParameterMapCtrl::add_pushed() {
	if(!parent->model_is_loaded()) return;
	if(!is_valid(current_par)) return;
	
	auto data_set = parent->data_set;
	int row = map_view.GetCursor();
	
	auto &parmap = data_set->parameters[current_par]->parmap_data;
	
	Parmap_Entry entry;
	int dataidx;
	if(row == -1) {
		row = map_view.GetCount();
		dataidx = data_index.back()+1;
		entry = parmap[data_index.back()];
	} else {
		dataidx = data_index[row];
		entry = parmap[dataidx];
	}
	parmap.insert(parmap.begin() + dataidx, entry);
	
	show_at_row(row, dataidx, entry);
	
	// Have to update the data indexes of the ones behind since we inserted an element
	for(int idx = row+1; idx < data_index.size(); ++idx)
		data_index[idx]++;
	
	propagate_edit();
}

void
ParameterMapCtrl::remove_pushed() {
	if(!parent->model_is_loaded()) return;
	if(!is_valid(current_par)) return;
	
	auto data_set = parent->data_set;
	
	if(map_view.GetCount() == 1) return;  // Not allowed to empty it.
	
	int row = map_view.GetCursor();
	if(row < 0) return;
	
	auto &parmap = data_set->parameters[current_par]->parmap_data;
	
	map_view.Remove(row);
	pos_ctrls.Remove(row);
	value_ctrls.Remove(row);
	parmap.erase(parmap.begin()+data_index[row]);
	data_index.erase(data_index.begin() + row);
	for(int r = row; r < data_index.size(); ++r)
		data_index[r]--;
	
	propagate_edit();
}

void
ParameterMapCtrl::pos_update(int row, double pos) {
	if(!parent->model_is_loaded()) return;
	if(!is_valid(current_par)) return;
	
	auto data_set = parent->data_set;
	auto &entry = data_set->parameters[current_par]->parmap_data[data_index[row]];

	try {
		// Replace the last index based on position.
		auto idx_set = entry.indexes.indexes.back().index_set;
		Token token;
		token.type = Token_Type::real;
		token.val_double = pos;
		
		int idxpos = (int)entry.indexes.indexes.size()-1;
		data_set->index_data.find_index(idx_set, &token, entry.indexes);
		
		// Want to erase the old one after so that it doesn't happen if there was an error.
		entry.indexes.indexes.erase(entry.indexes.indexes.begin()+idxpos);
		
		entry.pos = pos;
		propagate_edit();
		
		map_view.SetLineColor(row, Color(255, 255, 255));
	} catch(int) {
		//TODO: We should stop it from printing the error at all.
		// index_data.find_index should have optional error reporting
		global_error_stream.str("");
		map_view.SetLineColor(row, Color(205, 92, 92));
	}
}

void
ParameterMapCtrl::value_update(int row, double value) {
	if(!parent->model_is_loaded()) return;
	if(!is_valid(current_par)) return;
	
	auto data_set = parent->data_set;
	data_set->parameters[current_par]->parmap_data[data_index[row]].value = value;
	
	propagate_edit();
}

void
ParameterMapCtrl::propagate_edit() {
	
	auto par_data = parent->data_set->parameters[current_par];
	
	par_data->unpack_parameter_map(parent->data_set);
	
	read_single_parameter_data(parent->app, current_par_app, par_data);
	
	parent->params.changed_since_last_save = true;
	parent->params.refresh_parameter_view(true);
}