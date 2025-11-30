#pragma once
#include "const.h"

// 配置节的信息
struct SectionInfo {
	SectionInfo() {

	}
	~SectionInfo() {
		_section_datas.clear(); 
	}
	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas; 
	}

	SectionInfo& operator = (const SectionInfo& src) { 
		if (&src == this) { // 不允许自己拷贝自己
			return *this;
		}
		this->_section_datas = src._section_datas;
		return *this; // 返回当前对象的本身
	}

	std::map<std::string, std::string> _section_datas; 
	std::string operator[] (const std::string& key) { 
		if (_section_datas.find(key) == _section_datas.end()) { // 没有找到
			return ""; 
		}
		return _section_datas[key]; 
	}
};

class ConfigMgr
{
public: 
	~ConfigMgr() {
		_config_map.clear(); 
	}
	SectionInfo operator[] (const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo(); // 返回的是默认构造，默认构造初始化为空
		}
		return _config_map[section]; 
	}

	// 为了方便读取配置文件，将ConfigMgr改为单例, 将它的构造函数变成私有，添加Inst函数
	static ConfigMgr& Inst() {
		// 第一次执行到这行代码时，才会创建 cfg_mgr 实例。之后无论调用多少次 Inst()，这个 cfg_mgr 都是同一个实例
		static ConfigMgr cfg_mgr;
		return cfg_mgr; 
	}

	ConfigMgr& operator=(const ConfigMgr&) = delete;

	ConfigMgr(const ConfigMgr&) = delete;
private: 
	std::map<std::string, SectionInfo> _config_map; 
	ConfigMgr(); 
};

