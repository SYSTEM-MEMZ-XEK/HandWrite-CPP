#ifndef CORE_HPP
#define CORE_HPP

#include "tools.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <random>
#include <functional>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QFile>
#include <optional>

namespace HandWrite {

/**
 * @brief 字符级别覆盖设置
 * 每个字段都是可选的 - 如果未设置，则使用默认值
 */
struct CharacterOverride {
    std::optional<int> fontSize;           // 覆盖字体大小（像素，缩放前）
    std::optional<double> perturbX;        // 覆盖横向偏移（缩放前）
    std::optional<double> perturbY;        // 覆盖纵向偏移（缩放前）
    std::optional<double> perturbTheta;    // 覆盖旋转角度（弧度）
    std::optional<Color> fillColor;        // 覆盖填充颜色
    
    bool isEmpty() const {
        return !fontSize.has_value() && 
               !perturbX.has_value() && 
               !perturbY.has_value() && 
               !perturbTheta.has_value() && 
               !fillColor.has_value();
    }
};

/**
 * @brief 带覆盖设置的字符范围
 * 用于对选中文本应用自定义属性
 */
struct CharacterOverrideRange {
    int startIndex;                        // 文本中的起始位置（从0开始）
    int endIndex;                          // 文本中的结束位置（包含）
    CharacterOverride override;
    
    bool contains(int index) const {
        return index >= startIndex && index <= endIndex;
    }
};

/**
 * @brief 手写生成的模板参数
 */
struct TemplateParams {
    int rate = 4;                          // 图片缩放倍率
    int paperWidth = 667;                  // 纸张宽度（像素）
    int paperHeight = 945;                 // 纸张高度（像素）
    std::string fontPath;                  // 字体文件路径
    int fontSize = 30;                     // 字体大小
    int lineSpacing = 70;                  // 行间距（像素）
    int wordSpacing = 1;                   // 字间距（像素）
    int topMargin = 10;                    // 上边距
    int bottomMargin = 10;                 // 下边距
    int leftMargin = 10;                   // 左边距
    int rightMargin = 10;                  // 右边距
    
    double lineSpacingSigma = 1.0;         // 行间距扰动标准差
    double fontSizeSigma = 1.0;            // 字体大小扰动标准差
    double wordSpacingSigma = 1.0;         // 字间距扰动标准差
    double perturbXSigma = 1.0;            // 横向笔画扰动标准差
    double perturbYSigma = 1.0;            // 纵向笔画扰动标准差
    double perturbThetaSigma = 0.05;       // 旋转扰动标准差（弧度）
    
    std::string startChars = "\"（[<";    // 不应出现在行尾的字符
    std::string endChars = "，。";          // 不应出现在行首的字符
    
    Color fillColor = Color(0, 0, 0, 255); // 字体填充颜色（黑色）
    Color backgroundColor = Color(0, 0, 0, 0); // 背景颜色（透明）
    
    // 字符级别覆盖
    std::vector<CharacterOverrideRange> charOverrides;
};

/**
 * @brief 单页渲染数据（用于并行渲染）
 */
struct PageRenderData {
    int pageIndex;
    std::vector<std::pair<QString, QFont>> lines;
    int scaledWidth;
    int scaledHeight;
    TemplateParams params;
    std::mt19937 rng;  // 每页有独立的随机数生成器以保证可重现性
    
    // 字符索引映射：行索引 -> (行内字符索引 -> 全局字符索引)
    // 用于查找字符覆盖设置
    std::vector<std::vector<int>> charIndexMap;
};

/**
 * @brief 手写生成器引擎
 * 
 * 实现核心手写生成算法，
 * 通过自然扰动渲染文本以模拟手写效果。
 */
class HandwriteGenerator {
public:
    HandwriteGenerator();
    ~HandwriteGenerator() = default;
    
    // 获取默认模板参数
    const TemplateParams& templateParams() const { return m_params; }
    TemplateParams& templateParams() { return m_params; }
    
    // 修改模板参数
    void modifyTemplateParams(const TemplateParams& params);
    void setPaperSize(int width, int height);
    void setFont(const std::string& path, int size);
    void setMargins(int top, int bottom, int left, int right);
    void setSpacing(int lineSpacing, int wordSpacing);
    void setColors(const Color& fill, const Color& background);
    void setPerturbations(double lineSpacing, double fontSize, double wordSpacing,
                          double perturbX, double perturbY, double perturbTheta);
    void setRate(int rate);
    
    // 从文本生成图片（单线程，用于兼容）
    std::map<int, std::string> generateImage(const std::string& text, const std::string& outputDir = "outputs");
    
    // 生成预览（直接返回QImage）
    std::vector<QImage> generatePreview(const std::string& text);
    
    // 并行生成方法
    std::vector<QImage> generatePreviewParallel(const std::string& text, int threadCount = 0);
    std::map<int, std::string> generateImageParallel(const std::string& text, 
                                                      const std::string& outputDir = "outputs",
                                                      int threadCount = 0,
                                                      std::function<void(int, int)> progressCallback = nullptr);
    
    // 渲染单页的静态方法（可从任意线程调用）
    static QImage renderPageStatic(const PageRenderData& data);
    
    // 检测字体不支持的字符（生僻字）
    std::vector<QChar> findUnsupportedChars(const std::string& text);
    static std::vector<QChar> findUnsupportedCharsStatic(const std::string& text, 
                                                          const std::string& fontPath);
    
private:
    TemplateParams m_params;
    std::mt19937 m_rng;
    
    // 随机数生成器
    double gaussianRandom(double sigma);
    int gaussianRandomInt(double sigma);
    static double gaussianRandomStatic(double sigma, std::mt19937& rng);
    static int gaussianRandomIntStatic(double sigma, std::mt19937& rng);
    
    // 文本布局辅助函数
    std::vector<std::string> layoutText(const QString& text, const QFont& font, 
                                         int maxLineWidth, int scaledLineSpacing,
                                         int scaledWordSpacing = 0);
    bool isStartChar(QChar c) const;
    bool isEndChar(QChar c) const;
    static bool isStartCharStatic(QChar c, const std::string& startChars);
    static bool isEndCharStatic(QChar c, const std::string& endChars);
    
    // 渲染
    QImage renderPage(const std::vector<std::pair<QString, QFont>>& lines, int scaledWidth, int scaledHeight);
    void drawTextWithPerturbation(QPainter& painter, const QString& text, 
                                  qreal& x, qreal& y, const QFont& baseFont, int scaledWordSpacing);
    static void drawTextWithPerturbationStatic(QPainter& painter, const QString& text,
                                                qreal& x, qreal& y, const QFont& baseFont, 
                                                int scaledWordSpacing, const TemplateParams& params,
                                                std::mt19937& rng,
                                                int startCharIndex = 0,
                                                const std::vector<int>* charIndexMap = nullptr);
};

} // namespace HandWrite

#endif // CORE_HPP