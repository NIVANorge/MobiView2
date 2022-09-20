#include "ParameterCtrl.h"
#include "MobiView2.h"

using namespace Upp;

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
		//index_lock[idx]->WhenAction << SensitivityWindowUpdate;
		index_expand[idx]->Hide();
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
	for(auto index_set_id : model->modules[0]->index_sets) {
		auto index_set = model->find_entity<Reg_Type::index_set>(index_set_id);
		index_lock[idx]->Show();
		index_expand[idx]->Show();

		index_set_name[idx]->SetText(str(index_set->name));
		index_set_name[idx]->Show();
		
		for(String_View index_name : app->index_names[index_set_id.id])
			index_list[idx]->Add(str(index_name));
		
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

void ParameterCtrl::refresh(bool values_only) {
	if(!values_only) {
		parameter_view.Clear();
		parameter_view.Reset();
		parameter_view.NoVertGrid();
		
		parameter_controls.Clear();
		//CurrentParameterTypes.clear();
	}
	
	if(!parent->model_is_loaded()) return; // Should not be possible, but safety stopgap.
	/*
	int ExpandedSet = -1;
	ExpandedSetLocal = -1;
	
	SecondExpandedSetLocal = -1;
	
	*/
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
	auto par_group = parent->model->find_entity<Reg_Type::par_group>(par_group_id);
	
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx) {
		index_expand[idx]->Enable();
		index_list[idx]->Disable();
		
		if(expanded_set != index_sets[idx]) index_lock[idx]->Enable();
	}
	
	bool expanded_active = false;
	if(!par_group->parameters.empty()) {
		const std::vector<Entity_Id> &grp_index_sets = parent->app->parameter_structure.get_index_sets(par_group->parameters[0]);
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
	if(!expanded_active)
		expanded_set = invalid_entity_id;
	
	if(!values_only) {
		parameter_view.AddColumn(Id("__name"), "Name");
		
		if(is_valid(expanded_set)) {
			auto name = parent->app->model->find_entity<Reg_Type::index_set>(expanded_set)->name;
			parameter_view.AddColumn(Id("__index"), str(name));
		}
		
		parameter_view.AddColumn(Id("__value"), "Value");
		
		/*
		if(SecondExpandedSetLocal < 0)
		{
			// Otherwise regular editing of just one value
			Params.ParameterView.AddColumn(Id("__value"), "Value");
		}
		else
		{
			for(char *IndexName : SecondExpandedIndexSet)
				Params.ParameterView.AddColumn(Id(IndexName), IndexName);
		}
		*/
		
		parameter_view.AddColumn(Id("__min"), "Min");
		parameter_view.AddColumn(Id("__max"), "Max");
		parameter_view.AddColumn(Id("__unit"), "Unit");
		parameter_view.AddColumn(Id("__description"), "Description");
		
		if(is_valid(expanded_set))
			parameter_view.ColumnWidths("20 12 12 10 10 10 26");
		else
			parameter_view.ColumnWidths("20 12 10 10 10 38");
		/*
		if(SecondExpandedSetLocal < 0)
		{
			//TODO: Since these are user-configurable, it would be better to store the previous sizing of these and reuse that.
			if(ExpandedSet >= 0)
				Params.ParameterView.ColumnWidths("20 12 12 10 10 10 26");
			else
				Params.ParameterView.ColumnWidths("20 12 10 10 10 38");
		}
		if(SecondExpandedSetLocal >= 0)
		{
			//NOTE Hide the min, max, unit fields. We still have to add them since we use the
			//info stored in them some other places.
			int TabBase = 1 + (int)(ExpandedSetLocal >= 0) + SecondExpandedIndexSet.size();
			Params.ParameterView.HeaderObject().HideTab(TabBase + 0);
			Params.ParameterView.HeaderObject().HideTab(TabBase + 1);
			Params.ParameterView.HeaderObject().HideTab(TabBase + 2);
			Params.ParameterView.HeaderObject().HideTab(TabBase + 3);
		}*/
	}
	
	Indexed_Parameter par_data = {};
	if(!values_only) {
		par_data.valid = true;
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
	
	
	/*
	
	
	//NOTE: If the last two index sets are the same, display this as a row
	if(IndexSetNames.size() >= 2 && (strcmp(IndexSetNames[IndexSetNames.size()-1], IndexSetNames[IndexSetNames.size()-2]) == 0))
		SecondExpandedSetLocal = IndexSetNames.size()-1;
	
	if(ExpandedSetLocal == SecondExpandedSetLocal) ExpandedSetLocal--;  //NOTE: Ensure that we don't expand it twice in the same position.
	
	
	std::vector<char *> SecondExpandedIndexSet;
	if(SecondExpandedSetLocal >= 0)
	{
		char *ExpandedSetName = IndexSetNames[SecondExpandedSetLocal];
		
		uint64 ExpandedIndexesCount = ModelDll.GetIndexCount(DataSet, ExpandedSetName);
		SecondExpandedIndexSet.resize(ExpandedIndexesCount);
		ModelDll.GetIndexes(DataSet, ExpandedSetName, SecondExpandedIndexSet.data());
		
		if(CheckDllUserError()) return;
	}
	else
		SecondExpandedIndexSet.push_back((char *)"dummy");
	
	
	*/
	listed_pars.clear();
	
	Index_T exp_count = {expanded_set, 1};
	if(is_valid(expanded_set))
		exp_count = parent->app->index_counts[expanded_set.id];
	
	int row = 0;
	int ctrl_idx = 0;
	
	Color row_colors[2] = {{255, 255, 255}, {240, 240, 255}};
	
	for(Entity_Id par_id : par_group->parameters) {
		auto par = parent->model->find_entity<Reg_Type::parameter>(par_id);
		
		for(Index_T exp_idx = {expanded_set, 0}; exp_idx < exp_count; ++exp_idx) {
			if(!values_only) {
				par_data.id    = par_id;
				
				parameter_view.Add();
				
				if(is_valid(expanded_set))
					par_data.indexes[expanded_set.id] = exp_idx;
				
				listed_pars.push_back(par_data);
			}
			
			ValueMap row_data;
			
			if(is_valid(expanded_set))
				row_data.Set("__index", str(parent->app->index_names[expanded_set.id][exp_idx.index])); //TODO: should be an api for this in model_application
			
			bool show_additional = exp_idx.index == 0;
			
			// For expanded index sets, only show name, description, etc for the first row of
			// each parameter
			if(show_additional) {
				row_data.Set("__name", str(par->name));
				// TODO: generate strings for units
				//if(Unit) RowData.Set("__unit", Unit);
				if(par->description) row_data.Set("__description", str(par->description));
			}
			
			//for(char *SecondExpandedIndex : SecondExpandedIndexSet)
			//{
				Id value_column = "__value";
				/*if(SecondExpandedSetLocal >= 0)
				{
					ValueColumn = SecondExpandedIndex;
					Indexes[SecondExpandedSetLocal] = SecondExpandedIndex;
					if(!RefreshValuesOnly)
						Parameter.Indexes[SecondExpandedSetLocal].Name = SecondExpandedIndex;
				}*/
				
				s64 offset = parent->app->parameter_structure.get_offset(par_id, par_data.indexes);
				Parameter_Value val = *parent->app->data.parameters.get_value(offset);
				if(par->decl_type == Decl_Type::par_real) {
					row_data.Set(value_column, val.val_real);
					if(show_additional) {
						row_data.Set("__min", par->min_val.val_real);
						row_data.Set("__max", par->max_val.val_real);
					}
					
					if(!values_only) {
						parameter_controls.Create<EditDoubleNotNull>();
						parameter_controls[ctrl_idx].WhenAction = [row, ctrl_idx, this]() {
							EditDoubleNotNull *value_field = (EditDoubleNotNull *)&parameter_controls[ctrl_idx];
							if(IsNull(*value_field)) return;
							Parameter_Value val;
							val.val_real = (double)*value_field;
							parameter_edit(listed_pars[row], parent->app, val);
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
						parameter_controls[ctrl_idx].WhenAction = [row, ctrl_idx, this]() {
							EditInt64NotNullSpin *value_field = (EditInt64NotNullSpin *)&parameter_controls[ctrl_idx];
							if(IsNull(*value_field)) return;
							Parameter_Value val;
							val.val_integer = (int64)*value_field;
							parameter_edit(listed_pars[row], parent->app, val);
						};
					}
				} else if(par->decl_type == Decl_Type::par_bool) {
					row_data.Set(value_column, (int)val.val_boolean);

					if(!values_only) {
						parameter_controls.Create<Option>();
						parameter_controls[ctrl_idx].WhenAction = [row, ctrl_idx, this]() {
							Option *value_field = (Option *)&parameter_controls[ctrl_idx];
							Parameter_Value val;
							val.val_boolean = (bool)value_field->Get();
							parameter_edit(listed_pars[row], parent->app, val);
						};
					}
				} else if(par->decl_type == Decl_Type::par_datetime) {
					Time tm = convert_time(val.val_datetime);
					row_data.Set(value_column, tm);
					
					if(!values_only) {
						parameter_controls.Create<EditTimeNotNull>();
						parameter_controls[ctrl_idx].WhenAction = [row, ctrl_idx, this]() {
							EditTimeNotNull *value_field = (EditTimeNotNull *)&parameter_controls[ctrl_idx];
							Time tm = value_field->GetData();
							if(IsNull(tm)) return;
							Parameter_Value val;
							val.val_datetime = convert_time(tm);
							parameter_edit(listed_pars[row], parent->app, val);
						};
					}
				} else if(par->decl_type == Decl_Type::par_enum) {
					row_data.Set(value_column, val.val_integer);
					
					if(!values_only) {
						parameter_controls.Create<DropList>();
						DropList *enum_list = (DropList *)&parameter_controls[ctrl_idx];
						
						int64 key = 0;
						for(String_View name : par->enum_values)
							enum_list->Add(key++, str(name));
						enum_list->GoBegin();
						
						parameter_controls[ctrl_idx].WhenAction = [row, ctrl_idx, this]() {
							DropList *value_field = (DropList *)&parameter_controls[ctrl_idx];
							Parameter_Value val;
							val.val_integer = (int64)value_field->GetKey(value_field->GetIndex());
							parameter_edit(listed_pars[row], parent->app, val);
						};
					}
				}
				
				if(!values_only)
					parameter_view.SetCtrl(row, parameter_view.GetPos(value_column), parameter_controls[ctrl_idx]);
				
				++ctrl_idx;
			//}
			
			parameter_view.SetMap(row, row_data);
			
			// Alternating row colors for expanded indexes
			if(is_valid(expanded_set)) {
				Color &row_color = row_colors[row % 2];
				parameter_view.SetLineColor(row, row_color);
			}
			
			++row;
		}
	}
}

void ParameterCtrl::parameter_edit(Indexed_Parameter par_data, Model_Application *app, Parameter_Value val) {
	
	if(!par_data.valid || par_data.virt) {
		parent->log("Internal error: Trying to set value of parameter that is flagged as invalid or virtual.", true);
		return;
	}
	
	// Fetch the lock status of the lock controls.
	for(int idx = 0; idx < MAX_INDEX_SETS; ++idx)
		par_data.locks[idx] = (u8)(index_lock[idx]->IsEnabled() && (bool)index_lock[idx]->Get());
	
	try {
		set_parameter_value(par_data, app, val);
		changed_since_last_save = true;
	} catch(int) {}
	parent->log_warnings_and_errors();
}