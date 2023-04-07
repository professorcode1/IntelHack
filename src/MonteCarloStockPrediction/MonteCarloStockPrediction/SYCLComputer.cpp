#include "SYCLComputer.h"

/*float SYCLComputer::WightedAverageExponentialWeight(const std::vector<float>& data, const float base) {
	using namespace sycl;
	if (data.empty()) {
		return 0;
	}
	const int dataSize = data.size();
	auto dataSizeSycl = range<1>{ dataSize };
	buffer dataGpu(data);
	this->q.submit([&](handler& h) {
		accessor dataGpuAccessor(dataGpu, h, read_write);
	h.parallel_for(dataSizeSycl, [=](id<1> thread_id) {
		dataGpuAccessor[thread_id] *= std::pow(base, thread_id);
		});
		});
	for (int stride = 1 << (int)ceil(log2((double)dataSize) - 1); stride > 0; stride >>= 1) {
		this->q.wait();
		this->q.submit([&](handler& h) {
			accessor dataGpuAccessor(dataGpu, h, read_write);
		h.parallel_for(stride, [=](id<1> thread_id) {
			if (thread_id < stride && thread_id + stride < dataSize) {
				dataGpuAccessor[thread_id] += dataGpuAccessor[thread_id + stride];
			}
			});
			});
	}
	host_accessor result(dataGpu, read_only);

	return result[0];
}
*/