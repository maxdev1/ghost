#include "process.hpp"

g_kernquery_status processQuery(g_tid id, bool thread, g_kernquery_task_get_data& info)
{
	info = {};
	info.id = id;
	auto status = g_kernquery(thread ? G_KERNQUERY_TASK_GET_BY_ID : G_KERNQUERY_PROCESS_GET_BY_ID,
							  reinterpret_cast<uint8_t*>(&info));
	if(status == G_KERNQUERY_STATUS_SUCCESSFUL && !info.found)
		return G_KERNQUERY_STATUS_UNKNOWN_ID;
	return status;
}

static bool processTaskIds(std::vector<g_tid>& ids)
{
	g_kernquery_task_count_data count{};
	if(g_kernquery(G_KERNQUERY_TASK_COUNT, reinterpret_cast<uint8_t*>(&count)) != G_KERNQUERY_STATUS_SUCCESSFUL ||
	   count.count > UINT32_MAX - 16)
		return false;
	uint32_t capacity = count.count + 16;
	for(unsigned attempt = 0; attempt < 8; ++attempt)
	{
		ids.resize(capacity);
		g_kernquery_task_list_data list{};
		list.id_buffer = ids.data();
		list.id_buffer_size = capacity;
		if(g_kernquery(G_KERNQUERY_TASK_LIST, reinterpret_cast<uint8_t*>(&list)) != G_KERNQUERY_STATUS_SUCCESSFUL ||
		   list.filled_ids > capacity)
			return false;
		if(list.filled_ids < capacity)
		{
			ids.resize(list.filled_ids);
			return true;
		}
		if(capacity > UINT32_MAX / 2)
			return false;
		capacity *= 2;
	}
	return false;
}

bool processSnapshot(bool threads, std::vector<ProcessSample>& samples)
{
	std::vector<g_tid> ids;
	if(!processTaskIds(ids))
		return false;
	for(auto id: ids)
	{
		for(unsigned kind = 0; kind < (threads ? 2U : 1U); ++kind)
		{
			ProcessSample sample{};
			sample.thread = kind == 1;
			auto status = processQuery(id, sample.thread, sample.info);
			sample.sampledAt = g_nanos();
			if(status == G_KERNQUERY_STATUS_UNKNOWN_ID)
				continue;
			if(status != G_KERNQUERY_STATUS_SUCCESSFUL)
				return false;
			samples.push_back(sample);
		}
	}
	std::sort(samples.begin(), samples.end(),
			  [](const ProcessSample& a, const ProcessSample& b)
			  {
				  if(a.info.parent != b.info.parent)
					  return a.info.parent < b.info.parent;
				  if(a.thread != b.thread)
					  return !a.thread;
				  return a.info.id < b.info.id;
			  });
	return true;
}
