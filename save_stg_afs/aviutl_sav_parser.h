#include <string>
#include <cstdint>
#include <vector>
#include <iostream>
#include <memory>
#include <cstdarg>

static const std::string FILTER_CONFIG_HEADER = "AviUtl FilterConfigFile version 0.1";
static const std::string PROJECT_FILE_HEADER = "AviUtl ProjectFile version";

class FilterConfig {
public:
	std::string name;
	int head[3];
	uint32_t flags;
	int order;
	std::vector<int> track_data;
	std::vector<int> check_data;
	std::vector<char> other_data;

	FilterConfig() {};
	FilterConfig(std::string filter_name, const void *data) {
		name = filter_name;
		int *ptr = (int *)data;
		memcpy(head, ptr, sizeof(head));
		int track_count = ptr[3] / sizeof(track_data[0]);
		int check_count = ptr[4] / sizeof(check_data[0]);
		int idx = 5;
		int ext_data_bytes = (head[0] > 4) ? ptr[idx++] : 0;
		flags = (uint32_t)ptr[idx++];
		order = ptr[idx++];

		track_data.insert(track_data.end(), ptr + idx, ptr + idx + track_count); idx += track_count;
		check_data.insert(check_data.end(), ptr + idx, ptr + idx + check_count); idx += check_count;
		if (ext_data_bytes) {
			char *ext_ptr = (char *)(ptr + idx);
			other_data.insert(other_data.end(), ext_ptr, ext_ptr + ext_data_bytes);
		}
	};
	~FilterConfig() {
	};

	int get_size() {
		return 4 //最初のブロックサイズ(int)
			+ name.length() + 1 //フィルタ名
			+ (7 + (head[0] > 4)) * sizeof(head[0]) //ヘッダ部
			+ track_data.size() * sizeof(track_data[0])
			+ check_data.size() * sizeof(check_data[0])
			+ other_data.size() * sizeof(other_data[0]);
	}

	void print_data() {
		using namespace std;
		cout << strprintf("%8d", head[0]);
		cout << strprintf("%8d", head[1]);
		cout << strprintf("%8d", head[2]) << endl;
		cout << strprintf("size (track/check): %d / %d", track_data.size(), check_data.size()) << endl;
		if (head[0] > 4) {
			cout << strprintf("拡張データ領域    : %d", other_data.size()) << endl;
		}
		cout << strprintf("flags             : 0x%x", flags) << endl;
		cout << strprintf("順序              : %d", order) << endl;
		cout << "track             :";
		for (auto value : track_data) {
			cout << strprintf("%8d", value);
		}
		cout << endl;
		cout << "check             :";
		for (auto value : check_data) {
			cout << strprintf("%8d", value);
		}
		cout << endl;
		cout << "ext_data          :";
		for (auto value : other_data) {
			cout << strprintf("%3x", (uint8_t)value);
		}
		cout << endl;
	}

	std::vector<char> get_block() {
		const int block_size = get_size();
		const int filter_name_len = name.length();
		std::vector<char> block(block_size, 0);
		const int track_bytes = track_data.size() * sizeof(track_data[0]);
		const int check_bytes = check_data.size() * sizeof(check_data[0]);
		
		int idx = 0;
		auto set_data =[&](const void *data, int size) {
			memcpy(&block[idx], data, size);
			idx += size;
		};
		auto set_veci =[&](const std::vector<int>& data) {
			int size = data.size() * sizeof(data[0]);
			if (size > 0) {
				memcpy(&block[idx], &data[0], size);
				idx += size;
			}
		};
		auto set_vecc =[&](const std::vector<char>& data) {
			int size = data.size();
			if (size > 0) {
				memcpy(&block[idx], &data[0], size);
				idx += size;
			}
		};

		set_data(&block_size,  sizeof(block_size));
		set_data(&name[0],     filter_name_len + 1);
		set_data(head,         sizeof(head));
		set_data(&track_bytes, sizeof(track_bytes));
		set_data(&check_bytes, sizeof(check_bytes));
		if (head[0] > 4) {
			int ext_data_bytes = other_data.size();
			set_data(&ext_data_bytes, sizeof(ext_data_bytes));
		}
		set_data(&flags,         sizeof(flags));
		set_data(&order,         sizeof(order));
		set_veci(track_data);
		set_veci(check_data);
		set_vecc(other_data);

		return block;
	}
private:
	std::string strprintf(const char* format, ...) {
		std::va_list arg;
		va_start(arg, format);

		std::string ret;
		ret.resize(_vscprintf(format, arg) + 1);
		int n = vsprintf_s(&ret[0], ret.size(), format, arg);
		ret.resize(n);
		va_end(arg);
		return ret;
	}
};

class FilterConfigData {
private:
	int pos_start, pos_fin;
	std::string filepath;
	std::vector<std::unique_ptr<FilterConfig>> filter_list;
public:
	FilterConfigData() {};
	~FilterConfigData() {};
public:
	int parse(const std::vector<char>& file_data, int pos) {
		using namespace std;
		int ret = 0;
		if (0 == file_data.size() || 0 != FILTER_CONFIG_HEADER.compare(&file_data[0])) {
			ret = -1;
		} else {
			ret = FILTER_CONFIG_HEADER.length() + 1;
			for (int block_size = 0, idx = FILTER_CONFIG_HEADER.length() + 1; idx + 4 < (int)file_data.size(); idx += block_size) {
				block_size = *(int *)&file_data[idx];
				string filter_name = &file_data[idx+4];
				int jdx = 4 + filter_name.length() + 1;
				filter_list.push_back(std::unique_ptr<FilterConfig>(new FilterConfig(filter_name, &file_data[idx+jdx])));
				ret += block_size;
			}
			pos_start = pos;
			pos_fin = pos + ret;
		}
		return ret;
	}

	int get_pos_start() {
		return pos_start;
	}

	int get_pos_fin() {
		return pos_fin;
	}

	void write_list() {
		using namespace std;
		for (const auto& filter : filter_list) {
			cout << filter->get_size() << ":" << filter->name << endl;
			filter->print_data();
			cout << endl;
		}
	}

	FilterConfig *find_filter(std::string filter_name) {
		for (const auto& filter : filter_list) {
			if (0 == filter_name.compare(filter->name)) {
				return filter.get();
			}
		}
		return nullptr;
	}

	std::vector<char> get_block() {
		std::vector<char> data(FILTER_CONFIG_HEADER.length() + 1, 0);
		memcpy(&data[0], FILTER_CONFIG_HEADER.c_str(), FILTER_CONFIG_HEADER.length());
		for (const auto& filter_data : filter_list) {
			auto filter_block = filter_data->get_block();
			data.insert(data.end(), filter_block.begin(), filter_block.end());
		}
		return data;
	}
};
