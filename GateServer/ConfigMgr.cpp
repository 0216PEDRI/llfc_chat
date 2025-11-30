#include "ConfigMgr.h"

ConfigMgr::ConfigMgr()
{
	boost::filesystem::path current_path = boost::filesystem::current_path(); 
	boost::filesystem::path config_path = current_path / "config.ini"; 	// 配置文件路径: Boost.Filesystem重载/运算符实现路径拼接
	std::cout << "Config path: " << config_path << std::endl; 

	// 定义Boost.PropertyTree对象，用于存储解析后的配置数据
	boost::property_tree::ptree pt; 
	// 读取INI配置文件并解析到ptree中
	boost::property_tree::read_ini(config_path.string(), pt); 

    // 遍历配置文件中的所有配置节（Section）
    for (const auto& section_pair : pt) {
        // section_pair.first：配置节名称（如[VarifyServer]）
        const std::string& section_name = section_pair.first;
        // section_pair.second：配置节下的所有键值对数据
        const boost::property_tree::ptree& section_tree = section_pair.second;

        // 存储当前配置节下的键值对
        std::map<std::string, std::string> section_config;
        // 遍历当前配置节下的所有键值对
        for (const auto& key_value_pair : section_tree) {
            // key_value_pair.first：配置项名称（如Host/Port）
            const std::string& key = key_value_pair.first;
            /**
             * @note Boost.PropertyTree的设计特性：
             * 所有配置值（即使是简单字符串8080）都被封装在ptree中，以统一处理INI/XML/JSON等格式
             * 通过get_value<std::string>()提取实际的配置值
             */
            const std::string& value = key_value_pair.second.get_value<std::string>();
            section_config[key] = value;
        }

        // 创建SectionInfo对象，存储当前配置节的键值对数据
        SectionInfo sectionInfo;
        sectionInfo._section_datas = section_config;
        // 将配置节信息存入全局配置映射表
        _config_map[section_name] = sectionInfo;
    }

	// 调试输出：打印所有解析后的配置内容（可选，用于验证配置加载是否正确）
	for (const auto& section_entry : _config_map) {
		const std::string& section_name = section_entry.first;
		SectionInfo section_config = section_entry.second;
		std::cout << "[" << section_name << "]" << std::endl;
		for (const auto& key_value_pair : section_config._section_datas) {
			std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
		}
	}
}
