// ButterworthFilter: Implements a digital second order Butterworth filter for
// an Eigen::VectorXd type object

#ifndef SAI2_COMMON_BUTTERWORTH_FILTER_H_
#define SAI2_COMMON_BUTTERWORTH_FILTER_H_

#include <math.h>

#include <Eigen/Dense>
#include <iostream>
#include <stdexcept>

namespace Sai2Common {

/**
 * @brief Implements a digital second order Butterworth filter for an
 * Eigen::VectorXd type object
 *
 */
class ButterworthFilter {
public:
	/**
	 * @brief Construct a new Butterworth Filter object
	 *
	 * @param normalized_cutoff the normalized cutoff frequency of the filter
	 * (must be strictly between 0 and 0.5)
	 */
	ButterworthFilter(const double normalized_cutoff_freq);

	/**
	 * @brief Construct a new Butterworth Filter object
	 *
	 * @param sampling_rate the sampling rate of the signal to be filtered
	 * @param cutoff_freq the cutoff frequency of the filter (must be strictly
	 * between 0 and sampling_rate/2)
	 */
	ButterworthFilter(const double cutoff_freq, const double sampling_rate)
		: ButterworthFilter(cutoff_freq / sampling_rate) {};

	// disallow copy and assignement
	ButterworthFilter(const ButterworthFilter&) = delete;
	ButterworthFilter& operator=(const ButterworthFilter&) = delete;

	/**
	 * @brief Initialize the filter to a given value.
	 *
	 * @param initial_signal_values the initial value of the signal to be
	 * filtered
	 */
	void initializeFilter(const Eigen::VectorXd& initial_signal_values);

	/**
	 * @brief filter the new input value and return the filtered value. Each
	 * coefficient if filtered independently
	 *
	 * @param raw_input   the new input value
	 * @return Eigen::VectorXd  the filtered value
	 */
	Eigen::VectorXd update(const Eigen::VectorXd& raw_input);

	/**
	 * @brief Returns the normalized cutoff frequency of the filter
	 *
	 * @return The normalized cutoff frequency of the filter (should be strictly
	 * between 0 and 0.5)
	 */
	double getNormalizedCutoffFreq() const { return _normalized_cutoff; }

private:
	/**
	 * @brief Set the Cutoff Frequency of the filter.
	 *
	 * @param fc the normalized cutoff frequency of the filter (must be strictly
	 * between 0 and 0.5)
	 */
	void setCutoffFrequency(const double fc);

	// is intialized
	bool _is_initialized;

	// dimenson of the signal to be filtered
	size_t _dim;
	size_t _filter_order;

	// each column is a sample. Most recent sample at the left
	Eigen::MatrixXd _raw_buffer;
	Eigen::MatrixXd _filtered_buffer;

	// coeff associated with the most recent sample at the top
	Eigen::VectorXd _raw_coeff;
	Eigen::VectorXd _filtered_coeff;

	double _normalized_cutoff;
	double _pre_warp_cutoff;
};

}  // namespace Sai2Common

#endif	// SAI2_COMMON_BUTTERWORTH_FILTER_H_