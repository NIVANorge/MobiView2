#include "OptimizationWindow.h"
#include "MobiView2.h"
#include "Acor.h"

using namespace Upp;

#define IMAGECLASS IconImg6
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

MCMCProjectionCtrl::MCMCProjectionCtrl() {
	CtrlLayout(*this);
}

MCMCResultWindow::MCMCResultWindow(MobiView2 *parent) : parent(parent) {
	CtrlLayout(*this);
	
	SetRect(0, 0, 1200, 900);
	Title("MobiView MCMC results").Sizeable().Zoomable();
	
	view_result_summary.Add(result_summary.HSizePos().VSizePos(0, 30));
	view_result_summary.Add(push_write_map.SetLabel("MAP to main").LeftPos(0, 100).BottomPos(5, 25));
	view_result_summary.Add(push_write_median.SetLabel("Median to main").LeftPos(105, 100).BottomPos(5, 25));
	push_write_map.WhenPush    = THISBACK(map_to_main_pushed);
	push_write_median.WhenPush = THISBACK(median_to_main_pushed);
	
	view_projections.push_generate.WhenPush = THISBACK(generate_projections_pushed);
	view_projections.edit_samples.Min(1);
	view_projections.edit_samples.SetData(1000);
	view_projections.generate_progress.Hide();

	view_projections.confidence_edit.MinMax(0.0, 100.0).SetData(95.0);
	view_projections.plot_select_tab.Add(projection_plot_scroll.SizePos(), "Confidence interval");
	view_projections.plot_select_tab.Add(resid_plot_scroll.SizePos(), "Median residuals");
	view_projections.plot_select_tab.Add(autocorr_plot_scroll.SizePos(), "Median residual autocorr.");
	
	choose_plots_tab.Add(chain_plot_scroller.SizePos(), "Chain plots");
	choose_plots_tab.Add(triangle_plot_scroller.SizePos(), "Triangle plot");
	choose_plots_tab.Add(view_result_summary.SizePos(), "Parameter distribution summary");
	choose_plots_tab.Add(view_projections.SizePos(), "Result projections");
	
	choose_plots_tab.WhenSet << [&](){ refresh_plots(); };
	
	burnin_slider.WhenAction = THISBACK(burnin_slider_event);
	burnin_edit.WhenAction   = THISBACK(burnin_edit_event);
	
	burnin_slider.SetData(0);
	burnin_slider.MinMax(0,1);
	
	burnin_edit.SetData(0);
	burnin_edit.Min(0);
	
	burnin_edit.Disable();
	burnin_slider.Disable();
	
	AddFrame(tool);
	tool.Set(THISBACK(sub_bar));
	
	//PushHalt.WhenPush << [&](){ HaltWasPushed = true; };
	//PushHalt.SetImage(IconImg6::Remove());
}

void MCMCResultWindow::sub_bar(Bar &bar) {
	auto load = [&](){ bool success = load_results(); if(!success) PromptOK("There was an error in the file format."); }; //TODO: Handle error?
	
	bar.Add(IconImg6::Open(), load).Tip("Load data from file");
	bar.Add(IconImg6::Save(), THISBACK(save_results)).Tip("Save data to file");
}

void MCMCResultWindow::clean() {
	for(ScatterCtrl &plot : chain_plots) {
		plot.RemoveAllSeries();
		plot.Remove();
	}
	
	chain_plots.Clear();
	
	for(ScatterCtrl &plot : triangle_plots) {
		plot.RemoveSurf();
		plot.Remove();
	}
	
	triangle_plots.Clear();
	triangle_plot_ds.Clear();
	//triangle_plot_data.clear();
	
	for(ScatterCtrl &plot : histogram_plots) {
		plot.RemoveAllSeries();
		plot.Remove();
	}
	
	histogram_plots.Clear();
	//histograms.Clear();
	histogram_ds.clear();
	
	result_summary.Clear();

	acor_times.clear();
	//Debatable whether or not this should clear target plots.
}

void
MCMCResultWindow::refresh_plots(s64 cur_step) {
	if(!data) return;
	
	int show_plot = choose_plots_tab.Get();
	
	bool last_step = (cur_step == -1 || (data && cur_step == data->n_steps-1));
	
	if(show_plot == 0) {
		// Chain plots
		for(auto &plot : chain_plots) plot.Refresh();
		
	} else if(show_plot == 1) {
		// Triangle plots
		
		int burnin_val = (int)burnin[0];
		
		std::vector<std::vector<double>> par_values;
		s64 n_values = data->flatten(burnin_val, cur_step, par_values, true);
		
		if(n_values > 0) {
			double lower_quant = 0.025;
			double upper_quant = 0.975;
			
			int plot_idx = 0;
			if(data->n_pars > 1) {
				for(int par1 = 0; par1 < data->n_pars; ++par1) {
					std::vector<double> &par1_data = par_values[par1];
					
					double min_x = quantile_of_sorted(par1_data.data(), par1_data.size(), lower_quant);
					double max_x = quantile_of_sorted(par1_data.data(), par1_data.size(), upper_quant);
					double median_x = median_of_sorted(par1_data.data(), par1_data.size());
					double stride_x = (max_x - min_x)/(double)(distr_resolution);
					
					// It would be nice to use Upp::Histogram for this, but it is a bit
					// inconvenient with how you need to store a separate data source..
					
					auto        &dat  = histogram_ds[par1];
					ScatterCtrl &plot = histogram_plots[par1];
					
					double scale = 1.0;
					
					for(int idx = 0; idx < distr_resolution+1; ++idx) {
						dat.set_x(idx) = min_x + stride_x*(double)idx;
						dat.set_y(idx) = 0.0;
					}
					
					for(int walker = 0; walker < data->n_walkers; ++walker) {
						for(int step = burnin_val; step <= cur_step; ++step) {
							double val_x = (*data)(walker, par1, step);
							
							if(val_x < min_x || val_x > max_x)
								continue;
							
							int xx = std::min((int)std::floor((val_x - min_x)/stride_x), distr_resolution-1);
							xx = std::max(0, xx);
							dat.set_y(xx) += scale;
						}
					}
					
					plot.ZoomToFit(true, true);
					plot.SetMinUnits(0.0, 0.0);
					plot.SetMajorUnits(max_x - min_x, 1.0);
					plot.SetMajorUnitsNum(par1 == data->n_pars-1 ? 1 : 0, 0);
					plot.BarWidth(0.5*stride_x);
					plot.SetTitle(Format("%s = %.2f", free_syms[par1], median_x));
					plot.Refresh();
					
					for(int par2 = par1+1; par2 < data->n_pars; ++par2) {
						auto        &dat  = triangle_plot_ds[plot_idx];
						ScatterCtrl &plot = triangle_plots[plot_idx];
						
						std::vector<double> &par2_data = par_values[par2];
						
						double min_y = quantile_of_sorted(par2_data.data(), par2_data.size(), lower_quant);
						double max_y = quantile_of_sorted(par2_data.data(), par2_data.size(), upper_quant);
						
						double stride_y = (max_y - min_y)/(double)(distr_resolution);
						
						//TODO: could be optimized by just doing the steps since last update.
						dat.clear_z();
					
						double scale = 1.0; // Since we ZoomToFitZ, it shouldn't matter what the scale of Z is.
						
						for(int idx = 0; idx < distr_resolution+1; ++idx) {
							dat.set_x(idx) = min_x + stride_x*(double)idx;
							dat.set_y(idx) = min_y + stride_y*(double)idx;
						}
						
						for(int walker = 0; walker < data->n_walkers; ++walker) {
							for(int step = burnin_val; step <= cur_step; ++step) {
								double val_x = (*data)(walker, par1, step);
								double val_y = (*data)(walker, par2, step);
								
								if(val_x < min_x || val_x > max_x || val_y < min_y || val_y > max_y)
									continue;
								
								int xx = std::min((int)std::floor((val_x - min_x)/stride_x), distr_resolution-1);
								int yy = std::min((int)std::floor((val_y - min_y)/stride_y), distr_resolution-1);
								xx = std::max(0, xx);
								yy = std::max(0, yy);
								
								dat.set_z(xx, yy) += scale;
							}
						}
						
						plot.SetXYMin(min_x, min_y);
						plot.SetRange(max_x-min_x, max_y-min_y);
						plot.SetMinUnits(0.0, 0.0);
						plot.SetMajorUnits(max_x-min_x, max_y-min_y);
						plot.SetMajorUnitsNum(par2 == data->n_pars-1 ? 1: 0, par1 == 0 ? 1 : 0); //Annoyingly we have to reset this.
						
						plot.ZoomToFitZ();
						plot.Refresh();
						
						++plot_idx;
					}
				}
			}
		}
		
	}
	else if(show_plot == 2)
		refresh_result_summary(cur_step);
}

void MCMCResultWindow::resize_chain_plots() {
	int plot_count = chain_plots.size();
	
	int win_width  = GetRect().GetWidth()-50; // Allow for the scrollbar.
	
	int n_cols = 4;
	int n_rows = plot_count / 4 + (int)(plot_count % 4 != 0);
	
	int plot_width = win_width / n_cols;
	int plot_height = 160;

	for(int idx = 0; idx < plot_count; ++idx) {
		int xx = idx % n_cols;
		int yy = idx / n_cols;
		
		chain_plots[idx].LeftPos(xx*plot_width, plot_width);
		chain_plots[idx].TopPos(yy*plot_height, plot_height);
	}
	
	int tot_x = win_width;
	int tot_y = plot_height*n_rows + 50;
	chain_plot_scroller.ClearPane();
	Size tri_size(tot_x, tot_y);
	chain_plot_scroller.AddPane(view_chain_plots.LeftPos(0, tri_size.cx).TopPos(0, tri_size.cy), tri_size);
}

void MCMCResultWindow::begin_new_plots(MC_Data &data, std::vector<double> &min_bound, std::vector<double> &max_bound, int run_type) {
	//HaltWasPushed = false;
	if(run_type == 1) {  //NOTE: If we extend an earlier run, we keep the burnin that was set already, so we don't have to reset it now
		burnin_slider.SetData(0);
		burnin_edit.SetData(0);
		burnin[0] = 0; burnin[1] = 0;
	}
	
	this->free_syms.clear();
	for(int idx = 0; idx < expr_pars.exprs.size(); ++idx) {
		if(!expr_pars.exprs[idx].get())
			free_syms << expr_pars.parameters[idx].symbol.data();
	}
	
	this->min_bound = min_bound;
	this->max_bound = max_bound;
	
	burnin_slider.Enable();
	burnin_edit.Enable();
	
	burnin_slider.MinMax(0, (int)data.n_steps);
	burnin_edit.Max(data.n_steps);
	
	for(int par = 0; par < data.n_pars; ++par) {
		chain_plots.Create<ScatterCtrl>();
		view_chain_plots.Add(chain_plots.Top());
	}
	
	Color chain_color(0, 0, 0);
	Color burnin_color(255, 0, 0);
	
	burnin_plot_y.resize(2*data.n_pars);
	
	for(int par = 0; par < data.n_pars; ++par) {
		ScatterCtrl &plot = chain_plots[par];
		plot.SetFastViewX(true);
		plot.SetSequentialXAll(true);
		
		plot.SetMode(ScatterDraw::MD_ANTIALIASED);
		plot.SetPlotAreaLeftMargin(35).SetPlotAreaBottomMargin(15).SetPlotAreaTopMargin(4).SetTitle(free_syms[par]); //NOTE: Seems like title height is auto added to top margin.
		plot.SetMouseHandling(true, false);
		plot.SetTitleFont(plot.GetTitleFont().Height(14));
		plot.SetReticleFont(plot.GetReticleFont().Height(8));
		
		for(int walker = 0; walker < data.n_walkers; ++walker)
			plot.AddSeries(&data(walker, par, 0), data.n_steps, 0.0, 1.0).ShowLegend(false).NoMark().Stroke(1.0, chain_color).Dash("").Opacity(0.4);
		
		burnin_plot_y[2*par]     = min_bound[par];
		burnin_plot_y[2*par + 1] = max_bound[par];
		
		plot.AddSeries((double*)&burnin[0], burnin_plot_y.data()+2*par, (int)2).ShowLegend(false).NoMark().Stroke(1.0, burnin_color).Dash("");
		
		plot.SetXYMin(0.0, min_bound[par]);
		plot.SetRange((double)data.n_steps, max_bound[par] - min_bound[par]);
		
		if(par > 0)
			plot.LinkedWith(chain_plots[0]);
	}
	
	resize_chain_plots();
	
	this->data = &data;
	
	if(data.n_pars > 1) {
		int plot_idx = 0;
		int n_plots = (data.n_pars*(data.n_pars - 1)) / 2;
		
		triangle_plots.InsertN(0, n_plots);
		histogram_plots.InsertN(0, data.n_pars);
	
		int dim              = 55; //Size of the plot area excluding margins
		int small_margin_y   = 5;
		int small_margin_x   = 15;
		int large_margin     = 40;
		int dim_x            = 0;
		int dim_y            = 0;
		int sum_dim_x        = 0;
		int sum_dim_y        = 0;
		
		int tot_x = data.n_pars*dim + large_margin + (2*data.n_pars - 1)*small_margin_x + 50;
		int tot_y = data.n_pars*dim + large_margin + (2*data.n_pars - 1)*small_margin_y + 50;
		triangle_plot_scroller.ClearPane();
		Size tri_size(tot_x, tot_y);
		triangle_plot_scroller.AddPane(view_triangle_plots.LeftPos(0, tri_size.cx).TopPos(0, tri_size.cy), tri_size);
		
		for(int par1 = 0; par1 < data.n_pars; ++par1) {
			//Add the histogram on the top of the column
			sum_dim_y = (dim + 2*small_margin_y)*par1;
			
			auto        &dat  = histogram_ds.Create(distr_resolution+1, distr_resolution+1);
			ScatterCtrl &plot = histogram_plots[par1];
			
			double min_x = min_bound[par1];
			double max_x = max_bound[par1];
			
			double stride_x = (max_x - min_x)/(double)(distr_resolution);

			for(int idx = 0; idx < distr_resolution+1; ++idx) {
				dat.set_x(idx) = min_x + stride_x*(double)idx;
				dat.set_y(idx) = 0.0;
			}
			
			plot.AddSeries(dat).PlotStyle<BarSeriesPlot>()
				.BarWidth(0.5*stride_x).NoMark().Stroke(1.0, chain_color).ShowLegend(false);
				
			plot.SetXYMin(min_x, 0.0);
			plot.SetRange(max_x-min_x, 1.0);
			
			plot.SetMinUnits(0.0, 0.0).SetMajorUnits(max_x - min_x, 1.0);
			
			int h_left          = small_margin_x;
			int h_right         = small_margin_x;
			int v_top           = small_margin_y;   //NOTE title height is not included in this number, and is added on top of it.
			int v_bottom        = small_margin_y;
			if(par1 == 0)
				h_left = large_margin;
			
			if(par1 == data.n_pars-1) {
				v_bottom = large_margin;
				plot.SetMajorUnitsNum(1, 0);
			} else
				plot.SetMajorUnitsNum(0, 0);
			
			plot.SetTitle(free_syms[par1]);
			plot.SetTitleFont(plot.GetTitleFont().Bold(false).Height(8));
			plot.SetReticleFont(plot.GetReticleFont().Bold(false).Height(8));
			plot.SetPlotAreaMargin(h_left, h_right, v_top, v_bottom);
			
			dim_x = dim+h_left+h_right;
			dim_y = dim+v_top+v_bottom + 10;  //NOTE: Extra +10 is because of added title, but this is hacky... Check how to compute exact height of title?
			
			view_triangle_plots.Add(plot.LeftPos(sum_dim_x, dim_x).TopPos(sum_dim_y, dim_y));
			plot.SetMouseHandling(false, false);
			
			sum_dim_y += dim_y;
			
			// Add the 2d joint distributions
			for(int par2 = par1+1; par2 < data.n_pars; ++par2) {
				auto        &dat  = triangle_plot_ds.Create(distr_resolution+1, distr_resolution+1);
				ScatterCtrl &plot = triangle_plots[plot_idx];
				
				double min_y = min_bound[par2];
				double max_y = max_bound[par2];
				double stride_y = (max_y - min_y)/(double)(distr_resolution);
			
				for(int idx = 0; idx < distr_resolution+1; ++idx) {
					dat.set_x(idx) = min_x + stride_x*(double)idx;
					dat.set_y(idx) = min_y + stride_y*(double)idx;
				}
				
				plot.AddSurf(dat);
				plot.SetXYMin(min_x, min_y).SetRange(max_x-min_x, max_y-min_y);
				plot.ShowRainbowPalette(false);
				
				String par1_str = Null;
				String par2_str = Null;
				
				int major_units_num_x = 0;
				int major_units_num_y = 0;
				
				int h_left            = small_margin_x;
				int h_right           = small_margin_x;
				int v_top             = small_margin_y;
				int v_bottom          = small_margin_y;
				
				if(par1 == 0) {
					par2_str = free_syms[par2];
					major_units_num_y = 1;
					h_left            = large_margin;
				}
				
				if(par2 == data.n_pars-1) {
					par1_str = free_syms[par1];
					major_units_num_x = 1;
					v_bottom          = large_margin;
				}
				
				plot.SetMinUnits(0.0, 0.0).SetMajorUnits(max_x-min_x, max_y-min_y);
				plot.SetLabels(par1_str, par2_str);
				plot.SetLabelsFont(plot.GetLabelsFont().Bold(false).Height(8));
				plot.SetMajorUnitsNum(major_units_num_x, major_units_num_y);
				
				plot.SetPlotAreaMargin(h_left, h_right, v_top, v_bottom);
			
				Color grid_color(255, 255, 255);
				plot.SetGridColor(grid_color);
				
				dim_x = dim+h_left+h_right;
				dim_y = dim+v_top+v_bottom;
				
				view_triangle_plots.Add(plot.LeftPos(sum_dim_x, dim_x).TopPos(sum_dim_y, dim_y));
				plot.SetMouseHandling(false, false);
				plot.SurfRainbow(WHITE_BLACK);
				plot.SetReticleFont(plot.GetReticleFont().Bold(false).Height(8));
			
				sum_dim_y += dim_y;
				
				++plot_idx;
			}
			sum_dim_x += dim_x;
		}
	}
}

void
MCMCResultWindow::refresh_result_summary(s64 cur_step) {
	result_summary.Clear();
	
	if(!data) return;
	s64 burnin_val = (int)burnin[0];
	if(cur_step < 0) cur_step = data->n_steps - 1;
		
	std::vector<std::vector<double>> par_values;
	int n_values = data->flatten(burnin_val, cur_step, par_values, false);
	
	if(n_values <= 0) return;
	
	int best_w = -1;
	int best_s = -1;
	data->get_map_index(burnin_val, cur_step, best_w, best_s);
	
	int precision = 3; //parent->stat_settings.settings.precision;
	
	std::vector<double> means(data->n_pars);
	std::vector<double> std_devs(data->n_pars);
	Array<String> syms;
	
	double acceptance = 100.0*(double)data->n_accepted / (double)(data->n_walkers*data->n_steps);
	result_summary.append(Format("Acceptance rate: %.2f%%&&", acceptance));
	
	String table = "{{1:1:1:1:1:1";
	for(double perc : parent->stat_settings.settings.percentiles) table << ":1";
	table << " [* Name]:: [* Int.acor.T.]:: [* Mean]:: [* Median]:: [* MAP]:: [* StdDev]";
	for(double perc : parent->stat_settings.settings.percentiles) table << ":: " << FormatF(perc, 1) << "%";
	
	bool compute_acor = false;
	if(cur_step == data->n_steps-1 && acor_times.empty()) {
		compute_acor = true;
		acor_times.resize(data->n_pars);
		acor_below_tolerance.resize(data->n_pars);
	}
	
	//NOTE: These are parameters to the autocorrelation time computation
	int c=5, tol=10; //TODO: these should probably be configurable..
	
	bool any_below_tolerance = false;
	for(int par = 0; par < data->n_pars; ++par) {
	
		auto vals = par_values[par]; // NOTE: copy so that we don't sort the original (or it will break covariance computation below)
		std::sort(vals.begin(), vals.end());
		
		double sz   = (double)vals.size();
		double mean = std::accumulate(vals.begin(), vals.end(), 0.0) / sz;
	    auto var_func = [mean, sz](double acc, const double& val) {
	        return acc + (val - mean)*(val - mean);
	    };
	    double var  = std::accumulate(vals.begin(), vals.end(), 0.0, var_func) / sz;
		double sd = std::sqrt(var);
		
		double median = median_of_sorted(vals.data(), vals.size());
		std::vector<double> percentiles(parent->stat_settings.settings.percentiles.size());
		for(int idx = 0; idx < percentiles.size(); ++idx)
			percentiles[idx] = quantile_of_sorted(vals.data(), vals.size(), parent->stat_settings.settings.percentiles[idx]*0.01);
		
		means[par] = mean;
		std_devs[par] = sd;
		
		String sym = free_syms[par];
		sym.Replace("_", "`_");
		syms << sym;
		
		bool below_tolerance = false;
		double acor;
		if(compute_acor) {
			acor = integrated_time(*data, par, 5, 10, &below_tolerance);
			acor_times[par] = acor;
			acor_below_tolerance[par] = below_tolerance;
		} else {
			acor = acor_times[par];
			below_tolerance = acor_below_tolerance[par];
		}
		if(below_tolerance) any_below_tolerance = true;
		
		String acor_str = FormatDouble(acor, precision);
		if(below_tolerance) acor_str = Format("[@R `*%s]", acor_str);
		
		table
			<< ":: " << sym
			<< ":: " << acor_str
			<< ":: " << FormatDouble(mean, precision)
			<< ":: " << FormatDouble(median, precision)
			<< ":: " << (best_w >= 0 && best_s >= 0 ? FormatDouble((*data)(best_w, par, best_s), precision) : "N/A")
			<< ":: " << FormatDouble(sd, precision);
		
		for(double perc : percentiles) table << ":: " << FormatDouble(perc, precision);
	}
	table << "}}&";
	if(any_below_tolerance) table << Format("[@R `*The chain is shorter than %d times the integrated autocorrelation time for this parameter. Consider running for more steps.]&", tol);
	table << "[* Table 1:] Statistics about the posterior distribution of the parameters. MAP = most accurate prediction. Percents are percentiles of the distribution. Int.acor.T.=(estimated) integrated autocorrelation time.&&";

	result_summary.append(table);

	
	table = "{{1:";
	for(int par = 0; par < data->n_pars-2; ++par) table << "1:";
	table << "1 ";
	
	for(int par = 0; par < data->n_pars-1; ++par)
		table << ":: " << syms[par];
		
	for(int par1 = 1; par1 < data->n_pars; ++par1) {
		table << ":: " << syms[par1];
		for(int par2 = 0; par2 < data->n_pars-1; ++par2)	{
			table << "::";
			if(par2 >= par1){ table << "@W "; continue; }
			
			double corr = 0.0;
			for(int step = 0; step < par_values[0].size(); ++step) {
				double val1 = par_values[par1][step];
				double val2 = par_values[par2][step];
				corr += (val1 - means[par1])*(val2 - means[par2]);
			}
			corr /= (std_devs[par1]*std_devs[par2]*(double)n_values);
			
			int r = 0, b = 0;
			if(corr > 0) r = (int)(255.0*corr/2.0);
			if(corr < 0) b = (int)(-255.0*corr/2.0);
			table << Format("@(%d.%d.%d) %.2f", 255-b, 255-r-b, 255-r, corr);
		}
	}
	table << "}}&[* Table 2:] Pearson correlation coefficients between the parameters in the posterior distribution.";

	result_summary.append(table);
}

void
MCMCResultWindow::map_to_main_pushed() {
	//TODO: Should check that parameter setup in optim. window still correspond to this
	//parameter data!
	
	if(!data) return;
	
	int best_w = -1;
	int best_s = -1;
	data->get_map_index((int)burnin[0], data->n_steps-1, best_w, best_s);
	
	if(best_w < 0 || best_s < 0) return;
	
	std::vector<double> pars(data->n_pars);
	for(int par = 0; par < data->n_pars; ++par)
		pars[par] = (*data)(best_w, par, best_s);
	
	set_parameters(&parent->app->data, expr_pars, pars);
	parent->log("Wrote MCMC MAP parameters to main dataset.");
	parent->run_model();
}

void
MCMCResultWindow::median_to_main_pushed() {
	//TODO: Should check that parameter setup in optim. window still correspond to this
	//parameter data!
	//TODO: This is unnecessarily redoing a lot of work, but it may not be a problem..

	if(!data) return;
	
	std::vector<std::vector<double>> par_values;
	s64 cur_step = data->n_steps-1;
	data->flatten((int)burnin[0], cur_step, par_values, true);
	
	std::vector<double> pars(data->n_pars);
	for(int par = 0; par < data->n_pars; ++par)
		pars[par] = median_of_sorted(par_values[par].data(), par_values[par].size());
	
	set_parameters(&parent->app->data, expr_pars, pars);
	parent->log("Wrote MCMC median parameters to main dataset.");
	parent->run_model();
}

void
MCMCResultWindow::burnin_edit_event() {
	int val = burnin_edit.GetData();
	
	if(data)
		val = std::min(val, (int)data->n_steps);
	
	burnin[0] = (double)val;
	burnin[1] = (double)val;
	
	burnin_slider.SetData(val);
	
	if(data)
		refresh_plots();
}

void
MCMCResultWindow::burnin_slider_event() {
	int val = burnin_slider.GetData();
	
	if(data)
		val = std::min(val, (int)data->n_steps);
	
	burnin[0] = (double)val;
	burnin[1] = (double)val;
	
	burnin_edit.SetData(val);
	
	if(data)
		refresh_plots();
}

void
MCMCResultWindow::generate_projections_pushed() {
	
	if(!data) return;
	
	int n_samples = view_projections.edit_samples.GetData();
	double conf = view_projections.confidence_edit.GetData();
	double min_conf = 0.5*(100.0-conf);
	double max_conf = 100.0-min_conf;
	bool parametric_only = view_projections.option_uncertainty.GetData();
	
	for(MyPlot &plot : projection_plots) {
		plot.clean(false);
		plot.Remove();
	}
	projection_plots.Clear();
	projection_plots.InsertN(0, targets.size());
	
	for(MyPlot &plot : resid_plots) {
		plot.clean(false);
		plot.Remove();
	}
	resid_plots.Clear();
	resid_plots.InsertN(0, targets.size());
	
	for(MyPlot &plot : resid_histograms) {
		plot.clean(false);
		plot.Remove();
	}
	resid_histograms.Clear();
	resid_histograms.InsertN(0, targets.size());
	
	for(MyPlot &plot : autocorr_plots) {
		plot.clean(false);
		plot.Remove();
	}
	autocorr_plots.Clear();
	autocorr_plots.InsertN(0, targets.size());
	
	projection_plot_scroll.ClearPane();
	resid_plot_scroll.ClearPane();
	autocorr_plot_scroll.ClearPane();
	
	int h_size = projection_plot_scroll.GetRect().Width() - 30;  // -30 is to make space for scrollbar
	
	int plot_height = 400;
	int accum_y = 0;
	
	int target_idx = 0;
	for(Optimization_Target &target : targets) {
		int h_size_small = (int)(0.60*(double)h_size);
		
		projection_plot_pane.Add(projection_plots[target_idx].LeftPos(0, h_size).TopPos(accum_y, plot_height));
		resid_plot_pane.Add     (resid_plots[target_idx].LeftPos(0, h_size_small).TopPos(accum_y, plot_height));
		resid_plot_pane.Add     (resid_histograms[target_idx].LeftPos(h_size_small, h_size-h_size_small).TopPos(accum_y, plot_height));
		autocorr_plot_pane.Add  (autocorr_plots[target_idx].LeftPos(0, h_size).TopPos(accum_y, plot_height));
		accum_y += plot_height;
		++target_idx;
	}
	Size pane_sz(h_size, accum_y);
	projection_plot_scroll.AddPane(projection_plot_pane.LeftPos(0, h_size).TopPos(0, accum_y), pane_sz);
	resid_plot_scroll.AddPane(resid_plot_pane.LeftPos(0, h_size).TopPos(0, accum_y), pane_sz);
	autocorr_plot_scroll.AddPane(autocorr_plot_pane.LeftPos(0, h_size).TopPos(0, accum_y), pane_sz);
	
	view_projections.generate_progress.Show();
	view_projections.generate_progress.Set(0, n_samples);
	
	int burnin_val = (int)burnin[0];
	
	std::vector<std::vector<double>> par_values;
	s64 cur_step = -1;
	int n_values = data->flatten(burnin_val, cur_step, par_values, false);
	
	Date_Time result_start = parent->app->data.get_start_date_parameter();
	Date_Time result_end   = parent->app->data.get_end_date_parameter();
	s64 result_ts = steps_between(result_start, result_end, parent->app->time_step_size) + 1;
	
	std::vector<std::vector<std::vector<double>>> data_block;
	data_block.resize(targets.size());
	for(int target = 0; target < targets.size(); ++target) {
		data_block[target].resize(n_samples+1);   //NOTE: the final +1 is to make space for a run with the median parameter set
		for(int sample = 0; sample < n_samples+1; ++sample)
			data_block[target][sample].resize(result_ts);
	}

	std::vector<Plot_Setup> plot_setups;
	for(auto &target : targets)
		plot_setups.push_back(target_to_plot_setup(target, parent->app));
	
	int max_threads = std::thread::hardware_concurrency();
	int n_workers = std::max(max_threads, 32);
	std::vector<std::thread> workers;
	
	Random_State rand_state;
	std::vector<Model_Data*> model_datas(n_workers);
	for(int worker = 0; worker < n_workers; ++worker)
		model_datas[worker] = parent->app->data.copy();
	
	// TODO: should instead use a thread pool / job system
	for(int sample_group = 0; sample_group < n_samples/n_workers+1; ++sample_group) {
		for(int worker = 0; worker < n_workers; ++worker) {
			int sample = sample_group*n_workers + worker;
			if(sample >= n_samples) break;
			
			int n_pars = data->n_pars;
			workers.push_back(std::thread([this, &model_datas, &rand_state, &data_block, &par_values, worker, sample, n_pars, n_values, result_ts, parametric_only]() {
				std::vector<double> pars(n_pars);
				{
					std::lock_guard<std::mutex> lock(rand_state.gen_mutex);
					std::uniform_int_distribution<int> dist(0, n_values);
					int draw = dist(rand_state.gen);
					for(int par = 0; par < n_pars; ++par) pars[par] = par_values[par][draw];
				}
				auto model_data = model_datas[worker];
				set_parameters(model_data, expr_pars, pars);
				run_model(model_data);
				for(int target_idx = 0; target_idx < targets.size(); ++target_idx) {
					auto &target = targets[target_idx];
					std::vector<double> &copy_to = data_block[target_idx][sample];
					for(s64 ts = 0; ts < result_ts; ++ts)
						copy_to[ts] = *model_data->results.get_value(target.sim_offset, ts);
					if(!parametric_only) {
						std::lock_guard<std::mutex> lock(rand_state.gen_mutex);
						std::vector<double> err_par(target.err_par_idx.size());
						for(int idx = 0; idx < err_par.size(); ++idx)
							err_par[idx] = pars[target.err_par_idx[idx]];
						add_random_error(copy_to.data(), result_ts, err_par.data(), (LL_Type)target.stat_type, rand_state.gen);
					}
				}
			}));
		}
		for(auto &worker : workers)
			if(worker.joinable()) worker.join();
		workers.clear();
	
		view_projections.generate_progress.Set(std::min((sample_group+1)*n_workers-1, n_samples));
		if(sample_group % 8 == 0)
			parent->ProcessEvents();
	}
	
	// NOTE: Run once with the median parameter set also
	// TODO: it is unnecessary to recompute the medians all over the place. Just do it once and
	// store them?
	std::vector<double> pars(data->n_pars);
	for(int par = 0; par < data->n_pars; ++par) {
		std::vector<double> par_vals = par_values[par];
		std::sort(par_vals.begin(), par_vals.end());
		pars[par] = median_of_sorted(par_vals.data(), par_vals.size());
	}
	set_parameters(model_datas[0], expr_pars, pars);
	run_model(model_datas[0]);
	
	for(int target_idx = 0; target_idx < targets.size(); ++target_idx) {
		auto &target = targets[target_idx];
		std::vector<double> &copy_to = data_block[target_idx][n_samples];
		for(s64 ts = 0; ts < result_ts; ++ts)
			copy_to[ts] = *model_datas[0]->results.get_value(target.sim_offset, ts);
	}
	
	std::vector<double> buffer(n_samples);
	for(int target_idx = 0; target_idx < targets.size(); ++target_idx) {
		auto &target = targets[target_idx];
		
		MyPlot &plot = projection_plots[target_idx];
		if(target_idx != 0) plot.LinkedWith(projection_plots[0]);
		plot.setup = plot_setups[target_idx];
		plot.setup.scatter_inputs = true;
		
		plot.compute_x_data(result_start, result_ts, parent->app->time_step_size);
		
		auto obsdata = target.obs_id.type == Var_Id::Type::series ? &model_datas[0]->series : &model_datas[0]->additional_series;
		s64 input_ts_offset = steps_between(obsdata->start_date, result_start, parent->app->time_step_size);
		
		// Hmm, it is a bit annoying to have to copy both these out.
		std::vector<double> obs_copy;
		obs_copy.reserve(result_ts);
		std::vector<double> filtered_obs;
		filtered_obs.reserve(result_ts);
		
		int n_obs = 0;
		int coverage = 0;
		for(s64 ts = 0; ts < result_ts; ++ts) {
			for(int sample = 0; sample < n_samples; ++sample)
				buffer[sample] = data_block[target_idx][sample][ts];
			
			std::sort(buffer.begin(), buffer.end());

			// This is a bit hacky... We do it like this since it is more convenient to add the
			// plot from a Model_Data the way that code is written currently.
			double upper = *model_datas[1]->results.get_value(target.sim_offset, ts) = quantile_of_sorted(buffer.data(), buffer.size(), max_conf*0.01);
			*model_datas[2]->results.get_value(target.sim_offset, ts) = median_of_sorted(buffer.data(), buffer.size());
			double lower = *model_datas[3]->results.get_value(target.sim_offset, ts) = quantile_of_sorted(buffer.data(), buffer.size(), min_conf*0.01);
			
			double obs = *obsdata->get_value(target.obs_offset, ts+input_ts_offset);
			
			if(std::isfinite(obs)) {
				++n_obs;
				if(obs >= lower && obs <= upper) ++coverage;
				
				filtered_obs.push_back(obs);
			}
			
			obs_copy.push_back(obs);
		}
		
		// Do it like this so that colors don't depend on the order we add the plots:
		Color median_color = plot.colors.next();
		Color obs_color    = plot.colors.next();
		Color upper_color  = plot.colors.next();
		Color lower_color  = plot.colors.next();
		
		plot.SetLabelY(" "); // otherwise it generates a label that we don't want.
		
		// TODO: "Median" etc. should not be a prefix, it should be the entire series name...
		add_single_plot(&plot, model_datas[1], parent->app, target.sim_id, target.indexes,
				result_ts, result_start, result_start, plot.x_data.data(), 0, 0, upper_color, false, Format("%.2f percentile ", max_conf), true);
		
		add_single_plot(&plot, model_datas[2], parent->app, target.sim_id, target.indexes,
				result_ts, result_start, result_start, plot.x_data.data(), 0, 0, median_color, false, "Median ", true);
				
		add_single_plot(&plot, model_datas[3], parent->app, target.sim_id, target.indexes,
				result_ts, result_start, result_start, plot.x_data.data(), 0, 0, lower_color, false, Format("%.2f percentile ", min_conf), true);
		
		add_single_plot(&plot, model_datas[0], parent->app, target.obs_id, target.indexes,
				result_ts, result_start, result_start, plot.x_data.data(), 0, 0, obs_color, false, Null, true);
		
		auto &sim_name = parent->app->state_vars[target.sim_id]->name;
		double coverage_percent = 100.0*(double)coverage/(double)n_obs;
		plot.SetTitle(Format("%s, Coverage: %.2f%%", sim_name.data(), coverage_percent));
		
		format_axes(&plot, Plot_Mode::regular, 0, result_start, parent->app->time_step_size);
		set_round_grid_line_positions(&plot, 1);
		
		/*************** Compute and plot standardized residuals **********************/
		
		
		MyPlot &resid_plot = resid_plots[target_idx];
	
		//NOTE: This is the Y values of the median parameter set, not to be confused with the
			//median of the Y values over all the parameter sets that we plotted above.
		std::vector<double> &y_of_median = data_block[target_idx][n_samples];
			
		//Ooops, it is important that 'pars' still hold the median parameter set here.
		std::vector<double> err_par(target.err_par_idx.size());
		for(int idx = 0; idx < err_par.size(); ++idx) err_par[idx] = pars[target.err_par_idx[idx]];
		
		std::vector<double> standard_residuals;
		standard_residuals.reserve(result_ts);
		compute_standard_residuals(obs_copy.data(), y_of_median.data(), result_ts, err_par.data(), (LL_Type)target.stat_type, standard_residuals);
		
		if(filtered_obs.size() != standard_residuals.size())
			fatal_error(Mobius_Error::internal, "Something went wrong with computing standard residuals.");
		
		auto &resid_series = resid_plot.series_data.Create<Data_Source_Owns_XY>(&filtered_obs, &standard_residuals);
		
		Color resid_color = resid_plot.colors.next();
		resid_plot.SetSequentialXAll(false);
		resid_plot.SetTitle(sim_name.data());
		//resid_plot.SetTitle("Standard residuals");
		resid_plot.AddSeries(resid_series).Stroke(0.0, resid_color).MarkColor(resid_color).MarkStyle<CircleMarkPlot>();
		resid_plot.SetLabelX("Simulated").SetLabelY("Standard residual").ShowLegend(false);
		resid_plot.ZoomToFit(true, true);
		set_round_grid_line_positions(&resid_plot, 0);
		set_round_grid_line_positions(&resid_plot, 1);
	
		
		double min_resid = *std::min_element(standard_residuals.begin(), standard_residuals.end());
		double max_resid = *std::max_element(standard_residuals.begin(), standard_residuals.end());
		
		MyPlot &resid_hist = resid_histograms[target_idx];
		auto &hist_series = resid_hist.series_data.Create<Data_Source_Owns_XY>(&filtered_obs, &standard_residuals);
		
		int n_bins_histogram = add_histogram(&resid_hist, &hist_series, min_resid, max_resid, standard_residuals.size(), Null, Null, resid_color);
		format_axes(&resid_hist, Plot_Mode::histogram, n_bins_histogram, result_start, parent->app->time_step_size);
		resid_hist.ShowLegend(false);
		resid_hist.SetTitle("Standard resid. distr.");
		
		
		/*************** Autocorrelation plot *****************/
		
		std::vector<double> acor;
		normalized_autocorr_1d(standard_residuals, acor);
		
		std::vector<double> acor_idx(acor.size()-1);
		std::iota(acor_idx.begin(), acor_idx.end(), 1.0);
		
		s64 acor_size = acor.size();
		acor.erase(acor.begin());    // NOTE: the first element is not to be plotted.
		
		MyPlot &acor_plot = autocorr_plots[target_idx];
		
		Color acor_color(0, 0, 0);
		auto &acor_series = acor_plot.series_data.Create<Data_Source_Owns_XY>(&acor_idx, &acor);
		acor_plot.AddSeries(acor_series).NoMark().Stroke(1.5, acor_color).Dash("").Legend("Autocorrelation of standardized residual");
		
		double conf_p = conf/100.0;
		double z = 0.5*std::erfc(0.5*(1.0-conf_p)/std::sqrt(2.0)); //NOTE: This is just the cumulative distribution function of the normal distribution evaluated at conf_p
		z /= std::sqrt((double)acor_size);
		
		add_line(&acor_plot, 1.0,  z, (double)acor_size,  z, acor_color);
		add_line(&acor_plot, 1.0, -z, (double)acor_size, -z, acor_color);
		
		acor_plot.ZoomToFit(true, false).SetMouseHandling(false, true).SetXYMin(Null, -1.0).SetRange(Null, 2.0);
		set_round_grid_line_positions(&acor_plot, 0);
		set_round_grid_line_positions(&acor_plot, 1);
		acor_plot.SetLabelY("Autocorrelation");
		acor_plot.SetLabelX("Lag");
		acor_plot.SetTitle(sim_name.data());
	}
	
	for(int worker = 0; worker < n_workers; ++worker)
		delete model_datas[worker];

	view_projections.generate_progress.Hide();
}


void
MCMCResultWindow::save_results() {
	
	/*
		TODO: Serialization should just be baked into the Mobius2 mcmc package.
	*/
	
	if(!data) return;
	
	FileSel sel;
	sel.Type("MCMC results", "*.mcmc");
	
	if(!parent->data_file.empty())
		sel.ActiveDir(GetFileFolder(parent->data_file.data()));
	else
		sel.ActiveDir(GetCurrentDirectory());
	
	sel.ExecuteSaveAs();
	std::string file_name = sel.Get().ToStd();
	
	if(file_name.empty()) return;
	
	try {
		auto file = open_file(file_name.data(), "w");
		
		fprintf(file, "%d %d %d %d %d\n", data->n_pars, data->n_walkers, data->n_steps, (int)burnin_edit.GetData(), data->n_accepted);
		
		for(int par = 0; par < data->n_pars; ++par) {
			String &par_name = free_syms[par];
			
			fprintf(file, "%s %g %g\n", par_name.ToStd().data(), min_bound[par], max_bound[par]);

			for(int step = 0; step < data->n_steps; ++step) {
				for(int walker = 0; walker < data->n_walkers; ++walker)
					fprintf(file, "%g\t", (*data)(walker, par, step));
				fprintf(file, "\n");
			}
		}
		fprintf(file, "logprob\n");
		for(int step = 0; step < data->n_steps; ++step)	{
			for(int walker = 0; walker  < data->n_walkers; ++walker)
				fprintf(file, "%g\t", data->score_value(walker, step));
			fprintf(file, "\n");
		}
		
		String json_data = parent->optimization_window.write_to_json_string();
		fprintf(file, "%s", json_data.ToStd().data());
	
		fclose(file);

		parent->log(Format("MCMC data saved to %s", file_name.data()));
	} catch(int) {
		parent->log_warnings_and_errors();
	}
}

bool
MCMCResultWindow::load_results() {
	
	//NOTE: Error handling is very rudimentary for now.
	
	if(!data) data = &parent->optimization_window.mc_data; //TODO: Maybe this window should own the data block instead..
	
	FileSel sel;
	sel.Type("MCMC results", "*.mcmc");
	
	if(!parent->data_file.empty())
		sel.ActiveDir(GetFileFolder(parent->data_file.data()));
	else
		sel.ActiveDir(GetCurrentDirectory());
	
	sel.ExecuteOpen();
	std::string file_name = sel.Get().ToStd();
	
	bool success = true;
	
	String_View file_data = {};
	try {
		file_data = read_entire_file(file_name.data());
		//auto file = open_file(file_name.data(), "w");
		clean();
		
		Token_Stream stream(file_name, file_data);
		
		s64 n_pars = stream.expect_int();
		s64 n_walkers = stream.expect_int();
		s64 n_steps = stream.expect_int();
		s64 n_burnin = stream.expect_int();
		s64 n_accepted = stream.expect_int();
		
		// NOTE: Because of how betgin_new_plots works, we can't construct the member vectors
		// directly, we have to pass them to the function.
		std::vector<double> min_bound(n_pars);
		std::vector<double> max_bound(n_pars);
		Vector<String>      free_syms;            // Hmm, seems like this goes unused since it begin_new_plots doesn't need it any more
		
		data->allocate(n_walkers, n_pars, n_steps);
		data->n_accepted = n_accepted;
		
		for(int par = 0; par < n_pars; ++par) {
			auto sym = stream.expect_identifier();
			free_syms.push_back(sym.data);
			min_bound[par] = stream.expect_real();
			max_bound[par] = stream.expect_real();
			
			for(int step = 0; step < n_steps; ++step) {
				for(int walker = 0; walker < n_walkers; ++walker)
					(*data)(walker, par, step) = stream.expect_real();
			}
		}
		
		auto str = stream.expect_identifier();
		if(str == "logprob") {
			
			for(int step = 0; step < n_steps; ++step) {
				for(int walker = 0; walker < n_walkers; ++walker)
					data->score_value(walker, step) = stream.expect_real();
			}
		
			// TODO: Feels like some of this stuff should be factored out
			
			String json_data(stream.remainder());
			parent->optimization_window.read_from_json_string(json_data);
			parent->optimization_window.mcmc_setup.push_extend_run.Enable();
			// TODO: Why is this not a part of read_from_json_string ?
			
			parent->optimization_window.expr_pars.set(parent->app, parent->optimization_window.parameters);
			parent->optimization_window.err_sym_fixup();
			
			expr_pars.copy(parent->optimization_window.expr_pars);
			targets    = parent->optimization_window.targets;
			
			begin_new_plots(*data, min_bound, max_bound, 1);
			burnin_slider.SetData(n_burnin);
			burnin_edit.SetData(n_burnin);
			burnin[0] = burnin[1] = (double)n_burnin;
			
			refresh_plots();
			
			parent->log(Format("MCMC data loaded from %s", file_name.data()));
		
		} else
			success = false;
	} catch(int) {
		success = false;
		parent->log_warnings_and_errors();
	}
	
	if(file_data.data) free(file_data.data);
	
	return success;
}