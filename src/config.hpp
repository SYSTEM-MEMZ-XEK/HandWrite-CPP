#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <optional>
#include <map>
#include <variant>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace HandWrite {

/**
 * @brief 简单的TOML风格配置解析器/写入器
 * 支持基本TOML功能：键值对、数组、内联表
 */
class Config {
public:
    using Value = std::variant<int, double, std::string, std::vector<int>, std::vector<double>>;
    
    Config() = default;
    explicit Config(const std::string& path);
    
    bool load(const std::string& path);
    bool save(const std::string& path);
    
    // 获取器
    std::optional<int> getInt(const std::string& key) const;
    std::optional<double> getDouble(const std::string& key) const;
    std::optional<std::string> getString(const std::string& key) const;
    std::optional<std::vector<int>> getIntArray(const std::string& key) const;
    std::optional<std::vector<double>> getDoubleArray(const std::string& key) const;
    
    // 设置器
    void set(const std::string& key, int value);
    void set(const std::string& key, double value);
    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, const std::vector<int>& value);
    void set(const std::string& key, const std::vector<double>& value);
    
    // 检查键是否存在
    bool has(const std::string& key) const;
    
    // 属性风格访问器（与Python版本匹配）
    // 宽度
    std::optional<int> width() const;
    void setWidth(int value);
    
    // 高度
    std::optional<int> height() const;
    void setHeight(int value);
    
    // TTF选择器（字体路径）
    std::optional<std::string> ttfSelector() const;
    void setTtfSelector(const std::string& value);
    
    // 字体大小
    std::optional<int> fontSize() const;
    void setFontSize(int value);
    
    // 行间距
    std::optional<int> lineSpacing() const;
    void setLineSpacing(int value);
    
    // 字符间距
    std::optional<int> charDistance() const;
    void setCharDistance(int value);
    
    // 边距
    std::optional<int> marginTop() const;
    void setMarginTop(int value);
    std::optional<int> marginBottom() const;
    void setMarginBottom(int value);
    std::optional<int> marginLeft() const;
    void setMarginLeft(int value);
    std::optional<int> marginRight() const;
    void setMarginRight(int value);
    
    // 颜色（RGBA数组）
    std::optional<std::vector<int>> charColor() const;
    void setCharColor(const std::vector<int>& value);
    std::optional<std::vector<int>> backgroundColor() const;
    void setBackgroundColor(const std::vector<int>& value);
    
    // 分辨率（倍率）
    std::optional<int> resolution() const;
    void setResolution(int value);
    
    // 扰动标准差
    std::optional<double> lineSpacingSigma() const;
    void setLineSpacingSigma(double value);
    std::optional<double> fontSizeSigma() const;
    void setFontSizeSigma(double value);
    std::optional<double> wordSpacingSigma() const;
    void setWordSpacingSigma(double value);
    std::optional<double> perturbXSigma() const;
    void setPerturbXSigma(double value);
    std::optional<double> perturbYSigma() const;
    void setPerturbYSigma(double value);
    std::optional<double> perturbThetaSigma() const;
    void setPerturbThetaSigma(double value);
    
private:
    std::map<std::string, Value> m_data;
    
    // TOML解析辅助函数
    std::string trim(const std::string& str) const;
    std::string valueToString(const Value& value) const;
};

} // namespace HandWrite

#endif // CONFIG_HPP