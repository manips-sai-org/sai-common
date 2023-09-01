#include <iostream>
#include <fstream>
#include <Eigen/Dense>

#include "filters/ButterworthFilter.h"

using namespace std;
using namespace Eigen;

int main(int argc, char** argv) {
	cout << endl
		 << "This example generates a 2d signal where the first dimension is a "
			"superposition of 2 sine waves of frequency 1 and 5 Hz "
			"respectively, and the second one is a superposition of 2 sine "
			"waves of frequency 1 and 10 Hz respectively. It then implements 2 "
			"Butterworth filters of cutoff frequency 2.5 and 6 Hz and generates "
			"a file with the taw and filtered signals that can be plotted with "
			"the provided python script."
		 << endl
		 << endl;

	// create the filters
	double sampling_freq = 1000.0;
	Sai2Common::ButterworthFilter filter1(2.5, sampling_freq); // create using actual cutoff and sampling time
	Sai2Common::ButterworthFilter filter2(0.006);

	// create the signal
	double t = 0.0;
	double dt = 1.0 / sampling_freq;
	double t_end = 3.0;
	Vector2d signal = Vector2d(0.0,1.0);
	Vector2d filtered_signal1;
	Vector2d filtered_signal2;

	// initialize only filter1, the other will be initialized automatically when calling the update function
	filter1.initializeFilter(signal);

	// create a text file with the signal and filtered signals
	ofstream signal_file;
	signal_file.open("example_signal.txt");
	signal_file << "time[1]\traw_signal[2]\tfiltered_signal_1[2]\tfiltered_signal_2[2]" << endl;

	while(t <= t_end) {
		// compute the signal value
		signal(0) = sin(2.0*M_PI*t) + 0.25 * sin(2.0*M_PI*5.0*t);
		signal(1) = 1.25 - cos(2.0*M_PI*t) - 0.25 * cos(2.0*M_PI*10.0*t);

		// filter the signal
		filtered_signal1 = filter1.update(signal);
		filtered_signal2 = filter2.update(signal);

		// write to file
		signal_file << t << "\t" << signal(0) << "\t" << signal(1) << "\t"
					<< filtered_signal1(0) << "\t" << filtered_signal1(1)
					<< "\t" << filtered_signal2(0) << "\t"
					<< filtered_signal2(1) << endl;
		t += dt;
	}

	signal_file.close();


	return 0;
}