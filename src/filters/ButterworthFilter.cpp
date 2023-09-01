// ButterworthFilter: Implements a digital second order Butterworth filter for
// an Eigen::VectorXd type object

#include "ButterworthFilter.h"

namespace Sai2Common {

ButterworthFilter::ButterworthFilter(const double normalized_cutoff) {
	_is_initialized = false;
	_filter_order = 2;

	_raw_coeff.setZero(_filter_order + 1);
	_filtered_coeff.setZero(_filter_order);

	setCutoffFrequency(normalized_cutoff);
}

void ButterworthFilter::initializeFilter(
	const Eigen::VectorXd& initial_signal_values) {
	_dim = initial_signal_values.size();

	_raw_buffer.setZero(_dim, _filter_order + 1);
	_filtered_buffer.setZero(_dim, _filter_order);

	_raw_buffer.col(0) = initial_signal_values;
	for (int i = 0; i < _filter_order; ++i) {
		_raw_buffer.col(i + 1) = initial_signal_values;
		_filtered_buffer.col(i) = initial_signal_values;
	}
	_is_initialized = true;
}

void ButterworthFilter::setCutoffFrequency(const double fc) {
	if (fc >= 0.5 || fc <= 0) {
		throw std::runtime_error(
			"ButterworthFilter. normalized cutoff frequency should be strictly "
			"between 0 and 0.5\n");
	}

	_normalized_cutoff = fc;
	_pre_warp_cutoff = tan(M_PI * fc);

	double gain = (1 / (_pre_warp_cutoff * _pre_warp_cutoff) +
				   sqrt(2) / _pre_warp_cutoff + 1);

	const double ita = 1.0 / tan(M_PI * fc);
	const double q = sqrt(2.0);

	_raw_coeff << 1, 2, 1;
	_raw_coeff /= gain;

	_filtered_coeff << 2 - 2 / (_pre_warp_cutoff * _pre_warp_cutoff),
		1 / (_pre_warp_cutoff * _pre_warp_cutoff) - sqrt(2) / _pre_warp_cutoff +
			1;
	_filtered_coeff /= gain;
}

Eigen::VectorXd ButterworthFilter::update(const Eigen::VectorXd& raw_input) {
	if (!_is_initialized) {
		initializeFilter(raw_input);
	}

	if (raw_input.size() != _dim) {
		throw std::runtime_error(
			"ButterworthFilter::update(). input size does not match the "
			"dimension of the "
			"filter\n");
	}

	for (int i = _filter_order; i > 0; i--) {
		_raw_buffer.col(i) = _raw_buffer.col(i - 1);
	}
	_raw_buffer.col(0) = raw_input;

	Eigen::VectorXd y =
		_raw_buffer * _raw_coeff - _filtered_buffer * _filtered_coeff;

	for (int i = _filter_order - 1; i > 0; i--) {
		_filtered_buffer.col(i) = _filtered_buffer.col(i - 1);
	}
	_filtered_buffer.col(0) = y;

	return y;
}

}  // namespace Sai2Common