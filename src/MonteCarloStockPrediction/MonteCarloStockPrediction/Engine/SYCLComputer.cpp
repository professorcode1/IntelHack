#include "SYCLComputer.h"



std::vector<std::vector<float>> SYCLComputer::predict(
	const float mu, const float sigma,
	const float starting_price, const int number_of_simulations,
	const int number_of_days
) {
	using namespace std;
	using namespace sycl;
	vector<vector<float>> result(number_of_simulations, vector<float>(number_of_days));
	buffer<float, 2> gpuData(range<2>{number_of_simulations, number_of_days});
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	const float drift = mu - sigma * sigma / 2;
	{
		this->q.submit([&](handler& h) {
			accessor resultAccessor(gpuData, h, read_write);


			h.parallel_for(range<1>(number_of_simulations), [=](id<1> thread_id) {
				oneapi::dpl::minstd_rand engine(seed, thread_id);
				oneapi::dpl::normal_distribution<float> normal(0.0f, 1.0f);
				const float return__ = drift + normal(engine) * sigma;
				resultAccessor[thread_id][0] = starting_price * std::exp(return__);
				});
			}).wait();

	}

	this->q.submit([&](handler& h) {

		accessor resultAccessor(gpuData, h, read_write);
		h.parallel_for(range<1>(number_of_simulations), [=](id<1> thread_id) {
			for (int day = 1; day < number_of_days; day++) {
				int global_thread_id = number_of_simulations * thread_id + day;
				oneapi::dpl::minstd_rand engine(seed, global_thread_id);
				oneapi::dpl::normal_distribution<float> normal(0.0f, 1.0f);
				const float return__ = drift + normal(engine) * sigma;
				const float last_price = resultAccessor[thread_id][day - 1];
				resultAccessor[thread_id][day] = last_price * std::exp(return__);
			}
			});
		}).wait();
		host_accessor resultHostAccessor(gpuData, read_only);
		for (int sim = 0; sim < number_of_simulations; sim++) {
			for (int day = 0; day < number_of_days; day++) {
				result[sim][day] = resultHostAccessor[sim][day];
			}
		}
		return result;
}