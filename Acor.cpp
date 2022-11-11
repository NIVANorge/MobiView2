
#include <complex>
#include <numeric>
#include <plugin/eigen/unsupported/Eigen/FFT>
#include <immintrin.h>

#include "Acor.h"

//NOTE: Code adapted from python,  https://github.com/dfm/emcee/blob/main/src/emcee/autocorr.py

//NOTE: Apart from the dependency on Eigen, it would have been nice to factor this over to
//Mobius2/src/support

inline int next_pow_2(int n) {
	return 1 << (sizeof(int) - _lzcnt_u64(n - 1));    // TODO: gcc equivalent is __builtin_clz
}

void
normalized_autocorr_1d(const std::vector<double> &chain, std::vector<double> &acor) {
	Eigen::FFT<double> fft;
	
	std::vector<double> series = chain;
	double mean = std::accumulate(series.begin(), series.end(), 0.0) / (double)series.size();
	for(double &d : series) d -= mean;
	
	series.resize(2*next_pow_2(series.size())); //NOTE: To avoid autocorrelation between the beginning and end.
	std::vector<std::complex<double>> transformed;
	fft.fwd(transformed, series);

	for(auto &d : transformed) d *= std::conj(d);
	std::vector<std::complex<double>> back;
	fft.inv(back, transformed);
	acor.resize(chain.size());
	for(int idx = 0; idx < acor.size(); ++idx) acor[idx] = back[idx].real() / back[0].real();

#if 0
	//NOTE: *MUCH* slower implementation.. :
	acor.resize(chain.size());
	int n = chain.size();
	for(int h = 0; h < n; ++h) {
		double sum = 0.0;
		for(int t = 0; t < n-h; ++t)
			sum += (chain[t] - mean)*(chain[t+h] - mean);
		acor[h] = sum / (double)n;
		acor[h] /= acor[0];
	}
#endif
}

double
integrated_time(MC_Data &data, int par, int c, int tol, bool *below_tolerance) {
	std::vector<double> acor(data.n_steps, 0.0);
	std::vector<double> acor_1d;
	std::vector<double> chain(data.n_steps);
	
	//Take the mean of all 1D autocorrelation series for each walker.
	for(int walker = 0; walker < data.n_walkers; ++walker) {
		for(int step = 0; step < data.n_steps; ++step) chain[step] = data(walker, par, step);
		normalized_autocorr_1d(chain, acor_1d);
		for(int step = 0; step < data.n_steps; ++step) acor[step] += acor_1d[step] / (double)data.n_walkers;
	}
	
	double cumulative = 0.0;
	double tau_est;
	for(int step = 0; step < data.n_steps; ++step) {
		cumulative += acor[step];
		tau_est = 2.0*cumulative - 1.0;
		
		if((double)step > (double)c*tau_est) break;
	}
	
	*below_tolerance = (double)tol * tau_est > (double)data.n_steps;
	return tau_est;
}