#ifndef _MobiView2_Acor_h_
#define _MobiView2_Acor_h_

#include <vector>

#include "support/monte_carlo.h"

void
normalized_autocorr_1d(const std::vector<double> &chain, std::vector<double> &acor);

double
integrated_time(MC_Data &data, int par, int c, int tol, bool *below_tolerance);

#endif
