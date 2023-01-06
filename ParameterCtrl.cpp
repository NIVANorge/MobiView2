#include "ParameterCtrl.h"
#include "MobiView2.h"

using namespace Upp;

#define IMAGECLASS IconImg8
#define IMAGEFILE <MobiView2/images.iml>
#include <Draw/iml.h>

ParameterCtrl::ParameterCtrl(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this);
	
	parameter_view.AddColumn("Name");
	parameter_view.AddColumn("Value");
	parameter_view.AddColumn("Min");
	parameter_view.AddColumn("Max");
	parameter_view.AddColumn("Unit");
	parameter_view.AddColumn("Description");
	
	parameter_view.ColumnWidths("20 12 10 10 10 38");
	
	index_set_name[0] = &index_set_name1;
	index_set_name[1] = &index_set_name2;
	index_set_name[2] = &index_set_name3;
	index_set_name[3] = &index_set_name4;
	index_set_name[4] = &index_set_name5;
	index_set_name[5] = &index_set_name6;
	
	index_list[0]    = &index_list1;
	index_list[1]    = &index_list2;
	index_list[2]    = &index_list3;
	index_list[3]    = &index_list4;
	index_list[4]    = &index_list5;
	index_list[5]    = &index_list6;
	
	index_lock[0]    = &index_lock1;
	index_lock[1]    = &index_lock2;
	index_lock[2]    = &index_lock3;
	index_lock[3]    = &index_lock4;
	index_lock[4]    = &index_lock5;
	index_lock[5]    = &index_lock6;
	
	index_expand[0]    = &index_expand1;
	index_expand[1]    = &index_expand2;
	index_expand[2]    = &index_expand3;
	index_expand[3]    = &index_expand4;
	index_expand[4]    = &index_expand5;
	index_expand[5]    = &index_expand6;
	
	for(size_t idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_set_name[idx]->Hide();
		index_list[idx]->Hide();
		index_list[idx]->Disable();
		index_list[idx]->WhenAction << [this](){ refresh(false); };
		
		index_lock[idx]->Hide();
		index_lock[idx]->SetImage(IconImg8::LockOpen(), IconImg8::Lock());
		//index_lock[idx]->WhenAction << SensitivityWindowUpdate;
		index_expand[idx]->Hide();
		index_expand[idx]->SetImage(IconImg8::Add(), IconImg8::Remove());
		index_expand[idx]->WhenAction << [this, idx](){ expand_index_set_clicked(idx); };
	}
}

void ParameterCtrl::clean() {
	parameter_view.Clear();
	
	for(size_t idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_list[idx]->Clear();
		index_list[idx]->Hide();
		index_lock[idx]->Hide();
		index_expand[idx]->Hide();
		index_set_name[idx]->Hide();
	}
	index_sets.clear();
}

void ParameterCtrl::build_index_set_selecters(Model_Application *app) {
	if(!app) return;
	auto model = app->model;
	
	//NOTE: this relies on all the index sets being registered in the "global module":
	index_sets.resize(MAX_INDEX_SETS, invalid_entity_id);
	int idx = 0;
	for(auto index_set_id : model->index_sets) {
		auto index_set = model->index_sets[index_set_id];
		index_lock[idx]->Show();
		index_expand[idx]->Show();

		index_set_name[idx]->SetText(index_set->name.data());
		index_set_name[idx]->Show();
		
		for(Index_T index = {index_set_id, 0}; index < app->index_counts[index_set_id.id]; ++index)
			index_list[idx]->Add(app->get_index_name(index));
		
		index_list[idx]->GoBegin();
		index_list[idx]->Show();
		
		index_sets[idx] = index_set_id;
		
		++idx;
		
		if(idx == MAX_INDEX_SETS) {
			parent->log("Model has more index sets than are supported by MobiView2!", true);
			break;
		}
	}
}

void ParameterCtrl::expand_index_set_clicked(int idx) {
	bool checked = index_expand[idx]->Get();
	if(checked) {
		index_lock[idx]->Disable();
		index_list[idx]->Disable();
		for(int idx2 = 0; idx2 < MAX_INDEX_SETS; ++idx2) {
			if(idx != idx2) {
				index_expand[idx2]->Set(0);
				index_lock[idx2]->Enable();
				index_list[idx2]->Enable();
			}
		}
	} else {
		index_lock[idx]->Enable();
		index_list[idx]->Enable();
	}
	refresh();
}

//TODO: If we column expand, the "Expand" checkbox should be inactivated.

void ParameterCtrl::refresh(bool values_only) {
	if(!values_only) {
		parameter_view.Clear();
		parameter_view.Reset();
		parameter_view.NoVertGrid();
		
		parameter_controls.Clear();
	}
	
	if(!parent->model_is_loaded()) return; // Should not be possible, but safety stopgap.
	
	Entity_Id expanded_set = invalid_entity_id;
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx)
		if(index_expand[idx]->Get()) expanded_set = index_sets[idx];
	
	Vector<int> selected_groups = parent->par_group_selecter.GetSel();
	if(selected_groups.empty()) return;
	
	// Note: multiselect is off on the par_group_selecter, so we should get at most one
	// selected.
	Upp::Ctrl *ctrl = ~parent->par_group_selecter.GetNode(selected_groups[0]).ctrl;
	if(!ctrl) return;
	
	Entity_Id par_group_id = reinterpret_cast<Entity_Node *>(ctrl)->entity_id;
	auto par_group = parent->model->par_groups[par_group_id];
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_expand[idx]->Enable();
		index_list[idx]->Disable();
		
		if(expanded_set != index_sets[idx]) index_lock[idx]->Enable();
	}
	
	bool expanded_active = false;
	bool column_expand   = false;
	
	if(!par_group->parameters.empty()) {
		const std::vector<Entity_Id> &grp_index_sets = parent->app->parameter_structure.get_index_sets(par_group->parameters[0]);
		
		int sz = (int)grp_index_sets.size()-1;
		if(sz >= 1 && grp_index_sets[sz] == grp_index_sets[sz-1]) {    // Two last index set dependencies are the same -> matrix parameter
			expanded_set = grp_index_sets[sz];
			column_expand = true;
			expanded_active = true;
		} else {
			for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
				auto index_set = index_sets[idx];
				if(!is_valid(index_set)) break;
				// See if this item on the list of index set controls corresponds to one of the
				// group index sets. In that case, enable the index set control.
				if(std::find(grp_index_sets.begin(), grp_index_sets.end(), index_set) != grp_index_sets.end()) {
					index_list[idx]->Enable();
					if(index_set == expanded_set) expanded_active = true;
				}
			}
		}
	}
	if(!expanded_active)
		expanded_set = invalid_entity_id;
	
	Index_T exp_count = {expanded_set, 1};
	if(is_valid(expanded_set))
		exp_count = parent->app->index_counts[expanded_set.id];
	
	if(!values_only) {
		parameter_view.AddColumn(Id("__name"), "Name");
		
		if(is_valid(expanded_set)) {
			auto &name = parent->app->model->index_sets[expanded_set]->name;
			parameter_view.AddColumn(Id("__index"), name.data());
		}
		
		if(!column_expand)
			parameter_view.AddColumn(Id("__value"), "Value");
		else {
			for(Index_T exp_idx = {expanded_set, 0}; exp_idx < exp_count; ++exp_idx) {
				auto &name = parent->app->get_index_name(exp_idx);
				//TODO: This breaks if somebody calls one of the indexes e.g. "__name".
				parameter_view.AddColumn(Id(name.data()), name.data());
			}
		}
		
		parameter_view.AddColumn(Id("__min"), "Min");
		parameter_view.AddColumn(Id("__max"), "Max");
		parameter_view.AddColumn(Id("__unit"), "Unit");
		parameter_view.AddColumn(Id("__description"), "Description");
		
		if(!column_expand) {
			if(is_valid(expanded_set))
				parameter_view.ColumnWidths("20 12 12 10 10 10 26");
			else
				parameter_view.ColumnWidths("20 12 10 10 10 38");
		} else {
			//NOTE Hide the min, max, unit fields. We still have to add them since we use the
			//info stored in them some other places.
			int tab_base = 2 + exp_count.index;
			parameter_view.HeaderObject().HideTab(tab_base + 0);
			parameter_view.HeaderObject().HideTab(tab_base + 1);
			parameter_view.HeaderObject().HideTab(tab_base + 2);
			parameter_view.HeaderObject().HideTab(tab_base + 3);
		}
	}
	
	Indexed_Parameter par_data;
	if(!values_only) {
		par_data.indexes.resize(MAX_INDEX_SETS, invalid_index);
		for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
			if(!is_valid(index_sets[idx])) break;
			std::string idx_name = index_list[idx]->Get().ToStd();
			par_data.indexes[idx] = parent->app->get_index(index_sets[idx], String_View(idx_name));
		}
		//NOTE: We don't store info about it being locked here, since that has to be
			//overridden later anyway (the lock status can have changed since the table was
			//constructed.
		par_data.locks.resize(MAX_INDEX_SETS, false);
	}
	
	if(!values_only)
		listed_pars.clear();
	
	int row = 0;
	int ctrl_idx = 0;
	int par_idx = 0;
	
	Color row_colors[2] = {{255, 255, 255}, {240, 240, 255}};
	
	Index_T exp_count_2 = exp_count;
	if(!column_expand)
		exp_count_2 = Index_T {expanded_set, 1};
	
	if(!values_only)
		listed_pars.resize(par_group->parameters.size()*exp_count.index);
	
	for(Entity_Id par_id : par_group->parameters) {
		auto par = parent->model->parameters[par_id];
		
		for(Index_T exp_idx = {expanded_set, 0}; exp_idx < exp_count; ++exp_idx) {
			
			if(!values_only) {
				parameter_view.Add();
				
				par_data.id    = par_id;
				if(is_valid(expanded_set))
					par_data.indexes[expanded_set.id] = exp_idx;
			}
			
			ValueMap row_data;
			
			if(is_valid(expanded_set))
				row_data.Set("__index", parent->app->get_index_name(exp_idx).data()); //TODO: should be an api for this in model_application
			
			bool show_additional = exp_idx.index == 0;
			
			// For expanded index sets, only show name, description, etc for the first row of
			// each parameter
			if(show_additional) {
				row_data.Set("__name", par->name.data());
				if(is_valid(par->unit)) {
					auto unit = parent->app->model->units[par->unit];
					std::string unit_str = unit->data.to_utf8();
					row_data.Set("__unit", unit_str.data());
				}
				if(!par->description.empty()) row_data.Set("__description", par->description.data());
			}
			
			if(!values_only)
				listed_pars[row].resize(exp_count_2.index);
			
			int col = 0;
			for(Index_T exp_idx_2 = {expanded_set, 0}; exp_idx_2 < exp_count_2; ++exp_idx_2) {
				
				Id value_column = "__value";
				
				if(column_expand) {
					value_column = Id(parent->app->get_index_name(exp_idx_2).data());
					if(!values_only)
						par_data.mat_col = exp_idx_2;
				}
				
				if(!values_only)
					listed_pars[row][col] = par_data;
				else
					par_data = listed_pars[row][col];
				
				
				// TODO: use the second index in the lookup!!
				s64 offset;
				if(!column_expand)
					offset = parent->app->parameter_structure.get_offset(par_id, par_data.indexes);
				else
					offset = parent->app->parameter_structure.get_offset(par_id, par_data.indexes, par_data.mat_col);
					
				Parameter_Value val = *parent->app->data.parameters.get_value(offset);
				if(par->decl_type == Decl_Type::par_real) {
					row_data.Set(value_column, val.val_real);
					if(show_additional) {
						row_data.Set("__min", par->min_val.val_real);
						row_data.Set("__max", par->max_val.val_real);
					}
					
					if(!values_only) {
						parameter_controls.Create<EditDoubleNotNull>();
						parameter_controls[ctrl_idx].WhenAction = [row, col, ctrl_idx, this]() {
							EditDoubleNotNull *value_field = (EditDoubleNotNull *)&parameter_controls[ctrl_idx];
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
						parameter_controls.Create<EditInt64NotNullSpin>();
						parameter_controls[ctrl_idx].WhenAction = [row, col, ctrl_idx, this]() {
							EditInt64NotNullSpin *value_field = (EditInt64NotNullSpin *)&parameter_controls[ctrl_idx];
							if(IsNull(*value_field)) return;
							Parameter_Value val;
							val.val_integer = (int64)*value_field;
							parameter_edit(listed_pars[row][col], parent->app, val);
						};
					}
				} else if(par->decl_type == Decl_Type::par_bool) {
					row_data.Set(value_column, (int)val.val_boolean);

					if(!values_only) {
						parameter_controls.Create<Option>();
						parameter_controls[ctrl_idx].WhenAction = [row, col, ctrl_idx, this]() {
							Option *value_field = (Option *)&parameter_controls[ctrl_idx];
							Parameter_Value val;
							val.val_boolean = (bool)value_field->Get();
							parameter_edit(listed_pars[row][col], parent->app, val);
						};
					}
				} else if(par->decl_type == Decl_Type::par_datetime) {
					Time tm = convert_time(val.val_datetime);
					row_data.Set(value_column, tm);
					
					if(!values_only) {
						parameter_controls.Create<EditTimeNotNull>();
						parameter_controls[ctrl_idx].WhenAction = [row, col, ctrl_idx, this]() {
							EditTimeNotNull *value_field = (EditTimeNotNull *)&parameter_controls[ctrl_idx];
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
						parameter_controls.Create<DropList>();
						DropList *enum_list = (DropList *)&parameter_controls[ctrl_idx];
						
						int64 key = 0;
						for(auto &name : par->enum_values)
							enum_list->Add(key++, name.data());
						enum_list->GoBegin();
						
						parameter_controls[ctrl_idx].WhenAction = [row, col, ctrl_idx, this]() {
							DropList *value_field = (DropList *)&parameter_controls[ctrl_idx];
							Parameter_Value val;
							val.val_integer = (int64)value_field->GetKey(value_field->GetIndex());
							parameter_edit(listed_pars[row][col], parent->app, val);
						};
					}
				}
				
				if(!values_only)
					parameter_view.SetCtrl(row, parameter_view.GetPos(value_column), parameter_controls[ctrl_idx]);
				
				++ctrl_idx;
				
				++col;
			}
			
			parameter_view.SetMap(row, row_data);
			// Alternating row colors for expanded indexes
			if(is_valid(expanded_set)) {
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
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx)
		par_data.locks[idx] = (u8)(index_lock[idx]->IsEnabled() && (bool)index_lock[idx]->Get());
	
	try {
		set_parameter_value(par_data, &app->data, val);
		changed_since_last_save = true;
	} catch(int) {}
	parent->log_warnings_and_errors();
	
	// TODO: Do we really want to do this here? Ideally it should be a part of Mobius2 itself
	// so that it also happens if done through other APIs.
	if(std::find(app->baked_parameters.begin(), app->baked_parameters.end(), par_data.id) != app->baked_parameters.end()) {
		parent->reload(true);
		parent->log("Model was recompiled due to a change in a constant parameter."); // Is there a better name than "constant parameter"?
	}
}

void
ParameterCtrl::set_locks(Indexed_Parameter &par) {
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx)
		par.locks[idx] = (u8)(index_lock[idx]->IsEnabled() && (bool)index_lock[idx]->Get());
}

Indexed_Parameter
ParameterCtrl::get_selected_parameter() {
	Indexed_Parameter result;
	result.id = invalid_entity_id;
	
	int row = parameter_view.GetCursor();
	if(row < 0 || row >= listed_pars.size()) return std::move(result);
	
	// TODO: This is hacky... Do we want to switch to GridCtrl for parameter editing instead?
	// Also doesn't work for OptimizationWindow right now, because the event then happens after
	// the ctrl loses focus.
	int col_count = listed_pars[0].size();
	int col = 0;
	for(int idx = 0; idx < col_count; ++idx)
		if(parameter_controls[row*col_count + idx].HasFocus()) col = idx;
	
	result = listed_pars[row][col];
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