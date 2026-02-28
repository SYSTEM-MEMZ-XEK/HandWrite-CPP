#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <filesystem>

namespace HandWrite {

/**
 * @brief RGBA颜色结构体
 */
struct Color {
    unsigned char r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
        : r(r), g(g), b(b), a(a) {}
    
    std::tuple<int, int, int, int> toTuple() const {
        return {r, g, b, a};
    }
    
    static Color fromTuple(const std::tuple<int, int, int, int>& t) {
        return Color(
            static_cast<unsigned char>(std::get<0>(t)),
            static_cast<unsigned char>(std::get<1>(t)),
            static_cast<unsigned char>(std::get<2>(t)),
            static_cast<unsigned char>(std::get<3>(t))
        );
    }
};

/**
 * @brief 纸张尺寸结构体
 */
struct PaperSize {
    int width;
    int height;
    std::string name;
    
    PaperSize() : width(667), height(945), name("默认") {}
    PaperSize(int w, int h, const std::string& n) : width(w), height(h), name(n) {}
};

/**
 * @brief HandWrite应用的基础工具类
 */
class BasicTools {
public:
    BasicTools();
    
    // 字体颜色字典
    const std::map<std::string, Color>& fontColors() const { return m_fontColors; }
    Color getFontColor(const std::string& name) const;
    
    // 背景颜色字典
    const std::map<std::string, Color>& backgroundColors() const { return m_backgroundColors; }
    Color getBackgroundColor(const std::string& name) const;
    
    // 分辨率倍率字典
    const std::map<std::string, int>& resolutionRates() const { return m_resolutionRates; }
    int getResolutionRate(const std::string& name) const;
    
    // 纸张尺寸模板
    const std::map<std::string, PaperSize>& paperSizes() const { return m_paperSizes; }
    PaperSize getPaperSize(const std::string& name) const;
    std::vector<std::string> getPaperSizeNames() const;
    
    // 从库目录获取TTF字体文件
    std::vector<std::string> getTtfFileNames() const;
    std::vector<std::string> getTtfFilePaths() const;
    std::pair<std::vector<std::string>, std::vector<std::string>> getTtfFiles() const;
    
    // 设置TTF库路径
    void setTtfLibraryPath(const std::string& path);
    
private:
    std::map<std::string, Color> m_fontColors;
    std::map<std::string, Color> m_backgroundColors;
    std::map<std::string, int> m_resolutionRates;
    std::map<std::string, PaperSize> m_paperSizes;
    std::string m_ttfLibraryPath;
    
    void initializeColors();
    void initializeResolutionRates();
    void initializePaperSizes();
};

} // namespace HandWrite

#endif // TOOLS_HPP