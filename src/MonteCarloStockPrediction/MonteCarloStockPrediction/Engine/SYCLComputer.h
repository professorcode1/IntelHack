#pragma once

#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <stdio.h> 
#include <random>
#include "Utilities.h"
#include <oneapi/dpl/random>


static auto exception_handler = [](sycl::exception_list e_list) {
	for (std::exception_ptr const& e : e_list) {
		try {
			std::rethrow_exception(e);
		}
		catch (std::exception const& e) {
#if _DEBUG
			std::cout << "Failure" << std::endl;
#endif
			std::terminate();
		}
	}
};

class SYCLComputer
{
private:
	sycl::queue q;
	const int gpuLocalMemory;
	const int maxWorkGroupSize;
	const int maxSubGroupCount;

	template <typename T>
	void copy_buffer(sycl::buffer<T>& from, sycl::buffer<T>& to) {
		using namespace sycl;
		const size_t buffer_length = from.size();
		this->q.submit([&](handler& h) {
			accessor fromAccessor(from, h, read_only);
			accessor toAccessor(to, h, write_only);
			h.parallel_for(range<1>{buffer_length}, [=](id<1> threadId) {
				toAccessor[threadId] = fromAccessor[threadId];
				});
			});
		this->q.wait();
	}
	template <typename T>
	void reduce_buffer_add(sycl::buffer<T>& buffer, const int dataSize) {
		using namespace sycl;
		for (int stride = 1 << (int)ceil(log2((double)dataSize) - 1); stride > 0; stride >>= 1) {
			this->q.wait();
			this->q.submit([&](handler& h) {
				accessor bufferAccessor(buffer, h, read_write);
				h.parallel_for(stride, [=](id<1> thread_id) {
					if (thread_id < stride && thread_id + stride < dataSize) {
						bufferAccessor[thread_id] += bufferAccessor[thread_id + stride];
					}
					});
				});
		}
		this->q.wait();
	}

	float IndexWeightedMeanCalculation(sycl::buffer<float>& meanCalculationBuffer, const float base) {
		using namespace sycl;
		const int dataSize = meanCalculationBuffer.get_size();
		auto dataSizeSycl = range<1>{ dataSize };
		if (dataSize == 0) {
			return 0.0;
		}
		this->q.submit([&](handler& h) {
			accessor meanCalculationBufferAccessor(meanCalculationBuffer, h, read_write);
			h.parallel_for(dataSizeSycl, [=](id<1> thread_id) {
				meanCalculationBufferAccessor[thread_id] *= std::pow(base, dataSize - (thread_id + 1));
				});
			});
		this->reduce_buffer_add<float>(meanCalculationBuffer, dataSize);
		host_accessor result(meanCalculationBuffer, read_only);
		const float num = result[0];
		const float den = geometric_mean(1, base, dataSize);
		return num / den;
	}

	float IndexWeightedVarianceCalculation(
		sycl::buffer<float>& VarianceCalculationBuffer, const float base,
		const float mean
	) {
		using namespace sycl;
		const int dataSize = VarianceCalculationBuffer.get_size();
		auto dataSizeSycl = range<1>{ dataSize };
		if (dataSize == 0) {
			return 0.0;
		}
		this->q.submit([&](handler& h) {
			accessor VarianceCalculationBufferAccessor(VarianceCalculationBuffer, h, read_write);
			h.parallel_for(dataSizeSycl, [=](id<1> thread_id) {
				const float weight = std::pow(base, dataSize - (thread_id + 1));
				VarianceCalculationBufferAccessor[thread_id] -= mean;
				VarianceCalculationBufferAccessor[thread_id] *= VarianceCalculationBufferAccessor[thread_id];
				VarianceCalculationBufferAccessor[thread_id] *= weight;
				});
			});
		this->reduce_buffer_add<float>(VarianceCalculationBuffer, dataSize);
		host_accessor result(VarianceCalculationBuffer, read_only);

		const float r_exp = result[0];
		const float l_exp_num = geometric_mean(1, base, dataSize);
		const float l_exp_den_l = l_exp_num * l_exp_num;
		const float l_exp_den_r = geometric_mean(1, base * base, dataSize);
		const float l_exp_den = l_exp_den_l - l_exp_den_r;
		const float sd = (l_exp_num * r_exp) / l_exp_den;
		return sqrt(sd);
	}

	void BufferInplacePercentDifference(sycl::buffer<float>& buf) {
		using namespace sycl;
		const size_t size = buf.size();
		this->q.submit([&](handler& h) {
			accessor bufferAccessor(buf, h, read_write);
			h.parallel_for(range<1>(size - 1), [=](id<1> thread_id) {
				bufferAccessor[thread_id + 1] = (bufferAccessor[thread_id + 1] - bufferAccessor[thread_id]) / bufferAccessor[thread_id + 1];
				});
			});
		this->q.wait();
		this->q.submit([&](handler& h) {
			accessor bufferAccessor(buf, h, read_write);
			h.single_task([=]() {bufferAccessor[0] = 0.0; });
			});
		this->q.wait();
	}
	void BufferInplaceLogPlusOne(sycl::buffer<float>& buf) {
		using namespace sycl;
		const size_t size = buf.size();
		this->q.submit([&](handler& h) {
			accessor bufferAccessor(buf, h, read_write);
			h.parallel_for(range<1>(size), [=](id<1> thread_id) {
				bufferAccessor[thread_id] = std::log(bufferAccessor[thread_id] + 1);
				});
			});
		this->q.wait();
	}
	std::pair<float, float> IndexWeightedAverageAndSigma(sycl::buffer<float>& data, const float base) {
		sycl::buffer<float> meanCalculationBuffer(data.size());
		this->copy_buffer(data, meanCalculationBuffer);
		sycl::buffer<float> VarianceCalculationBuffer(data.size());
		this->copy_buffer(data, VarianceCalculationBuffer);
		const float mean = IndexWeightedMeanCalculation(meanCalculationBuffer, base);
		const float variance = IndexWeightedVarianceCalculation(VarianceCalculationBuffer, base, mean);
		return std::make_pair(mean, variance);
	}
public:
	SYCLComputer() :
		q(sycl::gpu_selector_v, exception_handler),
		gpuLocalMemory{ q.get_device().get_info<sycl::info::device::local_mem_size>() },
		maxWorkGroupSize{ q.get_device().get_info<sycl::info::device::max_work_group_size>() },
		maxSubGroupCount{ q.get_device().get_info<sycl::info::device::max_num_sub_groups>() }
	{};
	std::pair<float, float> StockMuAndSigma(const std::vector<float>& dataSource, const float base)
	{
		sycl::buffer<float, 1> data(dataSource);
		this->BufferInplacePercentDifference(data);
		this->BufferInplaceLogPlusOne(data);
		return this->IndexWeightedAverageAndSigma(data, base);
	}
	std::vector<std::vector<float>> predict(
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
};
