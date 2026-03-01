#include "core.hpp"
#include <QDir>
#include <QFontDatabase>
#include <QThread>
#include <QtConcurrent>
#include <QThreadPool>
#include <algorithm>
#include <cmath>

namespace HandWrite {

// 将中文标点转换为英文标点
static QString convertChinesePunctuation(const QString& text) {
    QString result = text;
    result.replace(QChar(0x2018), QChar(','));  // ' -> ,
    result.replace(QChar(0x2019), QChar(','));  // ' -> ,
    result.replace(QChar(0x201C), QChar('"'));  // " -> "
    result.replace(QChar(0x201D), QChar('"'));  // " -> "
    result.replace(QChar(0x3000), QChar(' '));  // 全角空格 -> 半角空格
    result.replace(QChar(0x3001), QChar(','));  // 、 -> ,
    result.replace(QChar(0x3002), QChar('.'));  // 。 -> .
    result.replace(QChar(0xFF01), QChar('!'));  // ！ -> !
    result.replace(QChar(0xFF0C), QChar(','));  // ， -> ,
    result.replace(QChar(0xFF1A), QChar(':'));  // ： -> :
    result.replace(QChar(0xFF1B), QChar(';'));  // ； -> ;
    result.replace(QChar(0xFF1F), QChar('?'));  // ？ -> ?
    result.replace(QChar(0xFF08), QChar('('));  // （ -> (
    result.replace(QChar(0xFF09), QChar(')'));  // ） -> )
    result.replace(QChar(0xFF3B), QChar('['));  // 【 -> [
    result.replace(QChar(0xFF3D), QChar(']'));  // 】 -> ]
    result.replace(QChar(0x300A), QChar('<'));  // 《 -> <
    result.replace(QChar(0x300B), QChar('>'));  // 》 -> >
    return result;
}

HandwriteGenerator::HandwriteGenerator() {
    // 用随机种子初始化随机数生成器
    std::random_device rd;
    m_rng.seed(rd());
    
    // 设置默认字体路径
    auto ttfFiles = BasicTools().getTtfFiles();
    if (!ttfFiles.second.empty()) {
        m_params.fontPath = ttfFiles.second[0];
    }
}

void HandwriteGenerator::modifyTemplateParams(const TemplateParams& params) {
    m_params = params;
}

void HandwriteGenerator::setPaperSize(int width, int height) {
    m_params.paperWidth = width;
    m_params.paperHeight = height;
}

void HandwriteGenerator::setFont(const std::string& path, int size) {
    m_params.fontPath = path;
    m_params.fontSize = size;
}

void HandwriteGenerator::setMargins(int top, int bottom, int left, int right) {
    m_params.topMargin = top;
    m_params.bottomMargin = bottom;
    m_params.leftMargin = left;
    m_params.rightMargin = right;
}

void HandwriteGenerator::setSpacing(int lineSpacing, int wordSpacing) {
    m_params.lineSpacing = lineSpacing;
    m_params.wordSpacing = wordSpacing;
}

void HandwriteGenerator::setColors(const Color& fill, const Color& background) {
    m_params.fillColor = fill;
    m_params.backgroundColor = background;
}

void HandwriteGenerator::setPerturbations(double lineSpacing, double fontSize, double wordSpacing,
                                          double perturbX, double perturbY, double perturbTheta) {
    m_params.lineSpacingSigma = lineSpacing;
    m_params.fontSizeSigma = fontSize;
    m_params.wordSpacingSigma = wordSpacing;
    m_params.perturbXSigma = perturbX;
    m_params.perturbYSigma = perturbY;
    m_params.perturbThetaSigma = perturbTheta;
}

void HandwriteGenerator::setRate(int rate) {
    m_params.rate = rate;
}

double HandwriteGenerator::gaussianRandom(double sigma) {
    if (sigma <= 0) return 0.0;
    std::normal_distribution<double> dist(0.0, sigma);
    return dist(m_rng);
}

int HandwriteGenerator::gaussianRandomInt(double sigma) {
    return static_cast<int>(std::round(gaussianRandom(sigma)));
}

bool HandwriteGenerator::isStartChar(QChar c) const {
    return m_params.startChars.find(c.toLatin1()) != std::string::npos;
}

bool HandwriteGenerator::isEndChar(QChar c) const {
    return m_params.endChars.find(c.toLatin1()) != std::string::npos;
}

std::vector<std::string> HandwriteGenerator::layoutText(const QString& text, const QFont& font,
                                                         int maxLineWidth, int scaledLineSpacing,
                                                         int scaledWordSpacing) {
    std::vector<std::string> lines;
    QFontMetrics fm(font);
    
    // 将中文标点转换为英文标点
    QString convertedText = convertChinesePunctuation(text);
    
    // 为扰动留出一些边距（字体大小变化、位置扰动）
    // 估计扰动对宽度的最大影响
    int perturbationMargin = static_cast<int>(m_params.fontSizeSigma * m_params.rate * 0.5) +
                             static_cast<int>(m_params.perturbXSigma * m_params.rate) +
                             static_cast<int>(m_params.wordSpacingSigma * m_params.rate * 2);
    
    // 考虑字间距和扰动边距的有效行宽
    int effectiveWidth = maxLineWidth - perturbationMargin;
    if (effectiveWidth < maxLineWidth / 2) {
        effectiveWidth = maxLineWidth / 2;  // 确保最小宽度
    }
    
    QString currentLine;
    int currentWidth = 0;
    
    for (int i = 0; i < convertedText.length(); ++i) {
        QChar c = convertedText[i];
        
        // 处理换行符
        if (c == '\n') {
            lines.push_back(currentLine.toStdString());
            currentLine.clear();
            currentWidth = 0;
            continue;
        }
        
        int charWidth = fm.horizontalAdvance(c) + scaledWordSpacing;
        
        // 检查添加此字符是否超过行宽
        if (currentWidth + charWidth > effectiveWidth && !currentLine.isEmpty()) {
            // 检查不应出现在行首的结束字符
            if (isEndChar(c)) {
                // 将此字符添加到当前行
                currentLine += c;
                currentWidth += charWidth;
                lines.push_back(currentLine.toStdString());
                currentLine.clear();
                currentWidth = 0;
                continue;
            }
            
            // 检查不应出现在行尾的开始字符
            if (!currentLine.isEmpty() && isStartChar(currentLine.back())) {
                // 将最后一个字符移到新行
                QChar lastChar = currentLine.back();
                currentLine.chop(1);
                lines.push_back(currentLine.toStdString());
                currentLine = lastChar;
                currentWidth = fm.horizontalAdvance(lastChar) + scaledWordSpacing;
            } else {
                lines.push_back(currentLine.toStdString());
                currentLine.clear();
                currentWidth = 0;
            }
        }
        
        currentLine += c;
        currentWidth += charWidth;
    }
    
    // 添加剩余文本
    if (!currentLine.isEmpty()) {
        lines.push_back(currentLine.toStdString());
    }
    
    return lines;
}

void HandwriteGenerator::drawTextWithPerturbation(QPainter& painter, const QString& text,
                                                   qreal& x, qreal& y, const QFont& baseFont, int scaledWordSpacing) {
    QFontMetrics fm(baseFont);
    
    for (int i = 0; i < text.length(); ++i) {
        QChar c = text[i];
        
        // 检查字体是否支持该字符，不支持则显示空白
        if (!fm.inFont(c)) {
            // 使用字体大小作为空白宽度
            x += fm.height() * 0.6 + scaledWordSpacing + gaussianRandom(m_params.wordSpacingSigma * m_params.rate);
            continue;
        }
        
        // 应用扰动
        qreal perturbX = gaussianRandom(m_params.perturbXSigma * m_params.rate);
        qreal perturbY = gaussianRandom(m_params.perturbYSigma * m_params.rate);
        qreal perturbTheta = gaussianRandom(m_params.perturbThetaSigma);
        
        // 应用字体大小扰动（使用pixelSize保持一致性）
        QFont perturbedFont = baseFont;
        if (m_params.fontSizeSigma > 0) {
            int basePixelSize = baseFont.pixelSize();
            if (basePixelSize > 0) {
                int fontSizeDelta = gaussianRandomInt(m_params.fontSizeSigma * m_params.rate);
                int newPixelSize = qMax(1, basePixelSize + fontSizeDelta);
                perturbedFont.setPixelSize(newPixelSize);
            }
        }
        
        painter.save();
        painter.setFont(perturbedFont);
        
        // 应用旋转扰动
        if (std::abs(perturbTheta) > 0.001) {
            painter.translate(x + perturbX, y + perturbY);
            painter.rotate(perturbTheta * 180.0 / M_PI);
            painter.drawText(0, 0, QString(c));
        } else {
            painter.drawText(x + perturbX, y + perturbY, QString(c));
        }
        
        painter.restore();
        
        // 移动到下一个字符位置
        x += fm.horizontalAdvance(c) + scaledWordSpacing + gaussianRandom(m_params.wordSpacingSigma * m_params.rate);
    }
}

QImage HandwriteGenerator::renderPage(const std::vector<std::pair<QString, QFont>>& lines,
                                       int scaledWidth, int scaledHeight) {
    // 创建带背景色的图像
    QImage image(scaledWidth, scaledHeight, QImage::Format_ARGB32);
    image.fill(QColor(m_params.backgroundColor.r, m_params.backgroundColor.g, 
                      m_params.backgroundColor.b, m_params.backgroundColor.a));
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // 设置填充颜色
    QColor fillColor(m_params.fillColor.r, m_params.fillColor.g, 
                     m_params.fillColor.b, m_params.fillColor.a);
    painter.setPen(fillColor);
    
    // 计算内容边界
    qreal contentTop = m_params.topMargin * m_params.rate;
    qreal contentBottom = scaledHeight - m_params.bottomMargin * m_params.rate;
    qreal contentLeft = m_params.leftMargin * m_params.rate;
    
    // y从内容顶部+基线开始（第一行的基线位置）
    qreal y = contentTop;
    QFontMetrics firstFm(lines.empty() ? QFont() : lines[0].second);
    y += firstFm.ascent();
    
    for (const auto& [line, font] : lines) {
        QFontMetrics fm(font);
        
        // 检查此行是否适合（考虑下边距的下降）
        if (y + fm.descent() > contentBottom) {
            break;
        }
        
        qreal x = contentLeft;
        
        // 应用行间距扰动
        qreal lineSpacingPerturb = gaussianRandom(m_params.lineSpacingSigma * m_params.rate);
        
        painter.setFont(font);
        
        // 绘制带扰动的文本
        QString lineText = line;
        qreal currentX = x;
        qreal currentY = y;
        
        drawTextWithPerturbation(painter, lineText, currentX, currentY, font, 
                                 m_params.wordSpacing * m_params.rate);
        
        // 移动到下一行
        y += m_params.lineSpacing * m_params.rate + lineSpacingPerturb;
    }
    
    return image;
}

std::vector<QImage> HandwriteGenerator::generatePreview(const std::string& text) {
    std::vector<QImage> images;
    
    // 计算缩放后的尺寸
    int scaledWidth = m_params.paperWidth * m_params.rate;
    int scaledHeight = m_params.paperHeight * m_params.rate;
    int scaledLeftMargin = m_params.leftMargin * m_params.rate;
    int scaledRightMargin = m_params.rightMargin * m_params.rate;
    int scaledTopMargin = m_params.topMargin * m_params.rate;
    int scaledBottomMargin = m_params.bottomMargin * m_params.rate;
    
    int contentWidth = scaledWidth - scaledLeftMargin - scaledRightMargin;
    int contentHeight = scaledHeight - scaledTopMargin - scaledBottomMargin;
    
    // 加载字体
    QFont font;
    if (!m_params.fontPath.empty()) {
        QString fontPath = QString::fromStdString(m_params.fontPath);
        qDebug() << "正在加载字体:" << fontPath;
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            qDebug() << "字体家族:" << families;
            if (!families.isEmpty()) {
                font = QFont(families[0]);
            }
        } else {
            qDebug() << "无法加载字体:" << fontPath;
        }
    } else {
        qDebug() << "未指定字体路径，使用默认字体";
    }
    // 使用像素大小以获得更精确的缩放
    int pixelSize = m_params.fontSize * m_params.rate;
    font.setPixelSize(pixelSize);
    qDebug() << "Font pixel size:" << pixelSize;
    
    // 将文本布局成行
    QString qText = QString::fromStdString(text);
    std::vector<std::string> textLines = layoutText(qText, font, contentWidth, 
                                                     m_params.lineSpacing * m_params.rate,
                                                     m_params.wordSpacing * m_params.rate);
    
    // 计算每页行数
    QFontMetrics fm(font);
    int lineHeight = m_params.lineSpacing * m_params.rate;
    
    // 将行分割成页
    // y从contentTop + ascent开始（第一行基线）
    // 每一行增加lineSpacing
    // 需要确保每行的y + descent <= contentBottom
    std::vector<std::vector<std::pair<QString, QFont>>> pages;
    std::vector<std::pair<QString, QFont>> currentPage;
    qreal y = scaledTopMargin + fm.ascent();  // 第一行基线
    
    for (const auto& line : textLines) {
        // 检查此行是否适合（考虑下边距的下降）
        if (!currentPage.empty()) {
            // 不是第一行，需要添加行间距
            int perturbedLineSpacing = lineHeight + gaussianRandomInt(m_params.lineSpacingSigma * m_params.rate);
            qreal nextY = y + perturbedLineSpacing;
            
            // 检查下一行是否适合
            if (nextY + fm.descent() > scaledHeight - scaledBottomMargin) {
                // 开始新页
                pages.push_back(currentPage);
                currentPage.clear();
                y = scaledTopMargin + fm.ascent();  // 为新页重置y
            } else {
                y = nextY;
            }
        }
        
        // 检查页面上第一行是否适合
        if (currentPage.empty() && y + fm.descent() > scaledHeight - scaledBottomMargin) {
            // 即使一行也不适合，仍然添加（将被裁剪）
        }
        
        currentPage.push_back({QString::fromStdString(line), font});
    }
    
    // 添加剩余行
    if (!currentPage.empty()) {
        pages.push_back(currentPage);
    }
    
    // 渲染每一页
    for (const auto& pageLines : pages) {
        QImage pageImage = renderPage(pageLines, scaledWidth, scaledHeight);
        images.push_back(pageImage);
    }
    
    // 确保至少有一页
    if (images.empty()) {
        QImage emptyImage(scaledWidth, scaledHeight, QImage::Format_ARGB32);
        emptyImage.fill(QColor(m_params.backgroundColor.r, m_params.backgroundColor.g,
                               m_params.backgroundColor.b, m_params.backgroundColor.a));
        images.push_back(emptyImage);
    }
    
    return images;
}

std::map<int, std::string> HandwriteGenerator::generateImage(const std::string& text, 
                                                              const std::string& outputDir) {
    std::map<int, std::string> filePaths;
    
    // 创建输出目录
    QDir dir;
    if (!dir.exists(QString::fromStdString(outputDir))) {
        dir.mkpath(QString::fromStdString(outputDir));
    }
    
    // 清空输出目录中的现有文件
    QDir outputDirectory(QString::fromStdString(outputDir));
    QStringList files = outputDirectory.entryList(QDir::Files);
    for (const QString& file : files) {
        outputDirectory.remove(file);
    }
    
    // 生成图像
    std::vector<QImage> images = generatePreview(text);
    
    // 保存图像到文件
    for (size_t i = 0; i < images.size(); ++i) {
        QString filePath = QString::fromStdString(outputDir) + "/" + QString::number(i) + ".png";
        if (images[i].save(filePath, "PNG")) {
            filePaths[static_cast<int>(i)] = filePath.toStdString();
        }
    }
    
    return filePaths;
}

// 线程安全渲染的静态辅助方法
double HandwriteGenerator::gaussianRandomStatic(double sigma, std::mt19937& rng) {
    if (sigma <= 0) return 0.0;
    std::normal_distribution<double> dist(0.0, sigma);
    return dist(rng);
}

int HandwriteGenerator::gaussianRandomIntStatic(double sigma, std::mt19937& rng) {
    return static_cast<int>(std::round(gaussianRandomStatic(sigma, rng)));
}

bool HandwriteGenerator::isStartCharStatic(QChar c, const std::string& startChars) {
    return startChars.find(c.toLatin1()) != std::string::npos;
}

bool HandwriteGenerator::isEndCharStatic(QChar c, const std::string& endChars) {
    return endChars.find(c.toLatin1()) != std::string::npos;
}

// 查找字符覆盖设置的辅助函数
static CharacterOverride findCharOverride(int globalCharIndex, const std::vector<CharacterOverrideRange>& overrides) {
    for (const auto& range : overrides) {
        if (range.contains(globalCharIndex)) {
            return range.override;
        }
    }
    return CharacterOverride();  // 空覆盖
}

void HandwriteGenerator::drawTextWithPerturbationStatic(QPainter& painter, const QString& text,
                                                         qreal& x, qreal& y, const QFont& baseFont,
                                                         int scaledWordSpacing, const TemplateParams& params,
                                                         std::mt19937& rng,
                                                         int startCharIndex,
                                                         const std::vector<int>* charIndexMap) {
    QFontMetrics fm(baseFont);
    
    for (int i = 0; i < text.length(); ++i) {
        QChar c = text[i];
        
        // 检查字体是否支持该字符，不支持则显示空白
        if (!fm.inFont(c)) {
            // 使用字体大小作为空白宽度
            x += fm.height() * 0.6 + scaledWordSpacing + 
                 gaussianRandomStatic(params.wordSpacingSigma * params.rate, rng);
            continue;
        }
        
        // 获取字符覆盖设置（如果有）
        CharacterOverride override;
        if (charIndexMap && i < static_cast<int>(charIndexMap->size())) {
            int globalIndex = (*charIndexMap)[i];
            override = findCharOverride(globalIndex, params.charOverrides);
        }
        
        // 应用扰动（如果设置了覆盖则使用覆盖值，否则使用默认值）
        qreal perturbX = override.perturbX.has_value() 
            ? *override.perturbX * params.rate 
            : gaussianRandomStatic(params.perturbXSigma * params.rate, rng);
        qreal perturbY = override.perturbY.has_value() 
            ? *override.perturbY * params.rate 
            : gaussianRandomStatic(params.perturbYSigma * params.rate, rng);
        qreal perturbTheta = override.perturbTheta.has_value() 
            ? *override.perturbTheta 
            : gaussianRandomStatic(params.perturbThetaSigma, rng);
        
        // 应用字体大小扰动（如果设置了覆盖则使用覆盖值）
        QFont perturbedFont = baseFont;
        if (override.fontSize.has_value()) {
            // 使用覆盖的字体大小（已缩放）
            perturbedFont.setPixelSize(*override.fontSize * params.rate);
        } else if (params.fontSizeSigma > 0) {
            int basePixelSize = baseFont.pixelSize();
            if (basePixelSize > 0) {
                int fontSizeDelta = gaussianRandomIntStatic(params.fontSizeSigma * params.rate, rng);
                int newPixelSize = qMax(1, basePixelSize + fontSizeDelta);
                perturbedFont.setPixelSize(newPixelSize);
            }
        }
        
        // 设置填充颜色（如果设置了覆盖则使用覆盖值）
        QColor fillColor = override.fillColor.has_value()
            ? QColor(override.fillColor->r, override.fillColor->g, 
                     override.fillColor->b, override.fillColor->a)
            : QColor(params.fillColor.r, params.fillColor.g, 
                     params.fillColor.b, params.fillColor.a);
        
        painter.save();
        painter.setFont(perturbedFont);
        painter.setPen(fillColor);
        
        // 应用旋转扰动
        if (std::abs(perturbTheta) > 0.001) {
            painter.translate(x + perturbX, y + perturbY);
            painter.rotate(perturbTheta * 180.0 / M_PI);
            painter.drawText(0, 0, QString(c));
        } else {
            painter.drawText(x + perturbX, y + perturbY, QString(c));
        }
        
        painter.restore();
        
        // 移动到下一个字符位置
        x += fm.horizontalAdvance(c) + scaledWordSpacing + 
             gaussianRandomStatic(params.wordSpacingSigma * params.rate, rng);
    }
}

QImage HandwriteGenerator::renderPageStatic(const PageRenderData& data) {
    // 创建带背景色的图像
    QImage image(data.scaledWidth, data.scaledHeight, QImage::Format_ARGB32);
    image.fill(QColor(data.params.backgroundColor.r, data.params.backgroundColor.g,
                      data.params.backgroundColor.b, data.params.backgroundColor.a));
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // 设置填充颜色
    QColor fillColor(data.params.fillColor.r, data.params.fillColor.g,
                     data.params.fillColor.b, data.params.fillColor.a);
    painter.setPen(fillColor);
    
    // 计算内容边界
    qreal contentTop = data.params.topMargin * data.params.rate;
    qreal contentBottom = data.scaledHeight - data.params.bottomMargin * data.params.rate;
    qreal contentLeft = data.params.leftMargin * data.params.rate;
    
    // y从内容顶部+基线开始（第一行的基线位置）
    qreal y = contentTop;
    QFontMetrics firstFm(data.lines.empty() ? QFont() : data.lines[0].second);
    y += firstFm.ascent();
    
    std::mt19937 localRng = data.rng;  // 为此页复制随机数生成器
    
    for (size_t lineIdx = 0; lineIdx < data.lines.size(); ++lineIdx) {
        const auto& [line, font] = data.lines[lineIdx];
        QFontMetrics fm(font);
        
        // 检查此行是否适合（考虑下边距的下降）
        if (y + fm.descent() > contentBottom) {
            break;
        }
        
        qreal x = contentLeft;
        
        // 应用行间距扰动
        qreal lineSpacingPerturb = gaussianRandomStatic(data.params.lineSpacingSigma * data.params.rate, localRng);
        
        painter.setFont(font);
        
        // 绘制带扰动的文本
        QString lineText = line;
        qreal currentX = x;
        qreal currentY = y;
        
        // 获取此行的字符索引映射（如果有）
        const std::vector<int>* lineCharIndexMap = nullptr;
        if (lineIdx < data.charIndexMap.size()) {
            lineCharIndexMap = &data.charIndexMap[lineIdx];
        }
        
        drawTextWithPerturbationStatic(painter, lineText, currentX, currentY, font,
                                       data.params.wordSpacing * data.params.rate, data.params, localRng,
                                       0, lineCharIndexMap);
        
        // 移动到下一行
        y += data.params.lineSpacing * data.params.rate + lineSpacingPerturb;
    }
    
    return image;
}

std::vector<QImage> HandwriteGenerator::generatePreviewParallel(const std::string& text, int threadCount) {
    // 确定最佳线程数
    if (threadCount <= 0) {
        threadCount = QThread::idealThreadCount();
        if (threadCount <= 0) threadCount = 4;
    }
    
    // 计算缩放后的尺寸
    int scaledWidth = m_params.paperWidth * m_params.rate;
    int scaledHeight = m_params.paperHeight * m_params.rate;
    int scaledLeftMargin = m_params.leftMargin * m_params.rate;
    int scaledRightMargin = m_params.rightMargin * m_params.rate;
    int scaledTopMargin = m_params.topMargin * m_params.rate;
    int scaledBottomMargin = m_params.bottomMargin * m_params.rate;
    
    int contentWidth = scaledWidth - scaledLeftMargin - scaledRightMargin;
    int contentHeight = scaledHeight - scaledTopMargin - scaledBottomMargin;
    
    // 加载字体
    QFont font;
    if (!m_params.fontPath.empty()) {
        QString fontPath = QString::fromStdString(m_params.fontPath);
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                font = QFont(families[0]);
            }
        }
    }
    int pixelSize = m_params.fontSize * m_params.rate;
    font.setPixelSize(pixelSize);
    
    // 将文本布局成行
    QString qText = QString::fromStdString(text);
    std::vector<std::string> textLines = layoutText(qText, font, contentWidth,
                                                     m_params.lineSpacing * m_params.rate,
                                                     m_params.wordSpacing * m_params.rate);
    
    // 计算行高
    QFontMetrics fm(font);
    int lineHeight = m_params.lineSpacing * m_params.rate;
    
    // 将行分割成页并创建渲染数据
    std::vector<PageRenderData> pageDataList;
    std::vector<std::pair<QString, QFont>> currentPage;
    std::vector<std::vector<int>> currentPageCharIndexMap;  // 当前页的字符索引映射
    qreal y = scaledTopMargin + fm.ascent();  // 第一行基线
    
    std::random_device rd;
    std::mt19937 seedRng(rd());
    
    int globalCharIndex = 0;  // 跟踪原始文本中的全局字符位置
    
    for (const auto& line : textLines) {
        // 检查此行是否适合（考虑下边距的下降）
        if (!currentPage.empty()) {
            // 不是第一行，需要添加行间距
            int perturbedLineSpacing = lineHeight + gaussianRandomInt(m_params.lineSpacingSigma * m_params.rate);
            qreal nextY = y + perturbedLineSpacing;
            
            // 检查下一行是否适合
            if (nextY + fm.descent() > scaledHeight - scaledBottomMargin) {
                // 开始新页
                PageRenderData pageData;
                pageData.pageIndex = static_cast<int>(pageDataList.size());
                pageData.lines = currentPage;
                pageData.scaledWidth = scaledWidth;
                pageData.scaledHeight = scaledHeight;
                pageData.params = m_params;
                pageData.rng.seed(seedRng());
                pageData.charIndexMap = currentPageCharIndexMap;
                pageDataList.push_back(pageData);
                
                currentPage.clear();
                currentPageCharIndexMap.clear();
                y = scaledTopMargin + fm.ascent();  // 为新页重置y
            } else {
                y = nextY;
            }
        }
        
        // 为此行构建字符索引映射
        QString lineText = QString::fromStdString(line);
        std::vector<int> lineCharIndices;
        for (int i = 0; i < lineText.length(); ++i) {
            lineCharIndices.push_back(globalCharIndex++);
        }
        
        currentPage.push_back({lineText, font});
        currentPageCharIndexMap.push_back(lineCharIndices);
    }
    
    // 添加剩余行
    if (!currentPage.empty()) {
        PageRenderData pageData;
        pageData.pageIndex = static_cast<int>(pageDataList.size());
        pageData.lines = currentPage;
        pageData.scaledWidth = scaledWidth;
        pageData.scaledHeight = scaledHeight;
        pageData.params = m_params;
        pageData.rng.seed(seedRng());
        pageData.charIndexMap = currentPageCharIndexMap;
        pageDataList.push_back(pageData);
    }
    
    // 确保至少有一页
    if (pageDataList.empty()) {
        PageRenderData pageData;
        pageData.pageIndex = 0;
        pageData.scaledWidth = scaledWidth;
        pageData.scaledHeight = scaledHeight;
        pageData.params = m_params;
        pageData.rng.seed(seedRng());
        pageDataList.push_back(pageData);
    }
    
    // 使用QtConcurrent并行渲染页面
    QThreadPool::globalInstance()->setMaxThreadCount(threadCount);
    std::vector<QImage> images = QtConcurrent::blockingMapped<std::vector<QImage>>(
        pageDataList, renderPageStatic);
    
    return images;
}

std::map<int, std::string> HandwriteGenerator::generateImageParallel(const std::string& text,
                                                                      const std::string& outputDir,
                                                                      int threadCount,
                                                                      std::function<void(int, int)> progressCallback) {
    std::map<int, std::string> filePaths;
    
    // 创建输出目录
    QDir dir;
    if (!dir.exists(QString::fromStdString(outputDir))) {
        dir.mkpath(QString::fromStdString(outputDir));
    }
    
    // 清空输出目录中的现有文件
    QDir outputDirectory(QString::fromStdString(outputDir));
    QStringList files = outputDirectory.entryList(QDir::Files);
    for (const QString& file : files) {
        outputDirectory.remove(file);
    }
    
    // 确定最佳线程数
    if (threadCount <= 0) {
        threadCount = QThread::idealThreadCount();
        if (threadCount <= 0) threadCount = 4;
    }
    
    // 计算缩放后的尺寸
    int scaledWidth = m_params.paperWidth * m_params.rate;
    int scaledHeight = m_params.paperHeight * m_params.rate;
    int scaledLeftMargin = m_params.leftMargin * m_params.rate;
    int scaledRightMargin = m_params.rightMargin * m_params.rate;
    int scaledTopMargin = m_params.topMargin * m_params.rate;
    int scaledBottomMargin = m_params.bottomMargin * m_params.rate;
    
    int contentWidth = scaledWidth - scaledLeftMargin - scaledRightMargin;
    int contentHeight = scaledHeight - scaledTopMargin - scaledBottomMargin;
    
    // 加载字体
    QFont font;
    if (!m_params.fontPath.empty()) {
        QString fontPath = QString::fromStdString(m_params.fontPath);
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                font = QFont(families[0]);
            }
        }
    }
    int pixelSize = m_params.fontSize * m_params.rate;
    font.setPixelSize(pixelSize);
    
    // 将文本布局成行
    QString qText = QString::fromStdString(text);
    std::vector<std::string> textLines = layoutText(qText, font, contentWidth,
                                                     m_params.lineSpacing * m_params.rate,
                                                     m_params.wordSpacing * m_params.rate);
    
    // 计算行高
    QFontMetrics fm(font);
    int lineHeight = m_params.lineSpacing * m_params.rate;
    
    // 将行分割成页并创建渲染数据
    std::vector<PageRenderData> pageDataList;
    std::vector<std::pair<QString, QFont>> currentPage;
    std::vector<std::vector<int>> currentPageCharIndexMap;
    qreal y = scaledTopMargin + fm.ascent();  // 第一行基线
    
    std::random_device rd;
    std::mt19937 seedRng(rd());
    
    int globalCharIndex = 0;  // 跟踪原始文本中的全局字符位置
    
    for (const auto& line : textLines) {
        // 检查此行是否适合（考虑下边距的下降）
        if (!currentPage.empty()) {
            // 不是第一行，需要添加行间距
            int perturbedLineSpacing = lineHeight + gaussianRandomInt(m_params.lineSpacingSigma * m_params.rate);
            qreal nextY = y + perturbedLineSpacing;
            
            // 检查下一行是否适合
            if (nextY + fm.descent() > scaledHeight - scaledBottomMargin) {
                // 开始新页
                PageRenderData pageData;
                pageData.pageIndex = static_cast<int>(pageDataList.size());
                pageData.lines = currentPage;
                pageData.scaledWidth = scaledWidth;
                pageData.scaledHeight = scaledHeight;
                pageData.params = m_params;
                pageData.rng.seed(seedRng());
                pageData.charIndexMap = currentPageCharIndexMap;
                pageDataList.push_back(pageData);
                
                currentPage.clear();
                currentPageCharIndexMap.clear();
                y = scaledTopMargin + fm.ascent();  // 为新页重置y
            } else {
                y = nextY;
            }
        }
        
        // 为此行构建字符索引映射
        QString lineText = QString::fromStdString(line);
        std::vector<int> lineCharIndices;
        for (int i = 0; i < lineText.length(); ++i) {
            lineCharIndices.push_back(globalCharIndex++);
        }
        
        currentPage.push_back({lineText, font});
        currentPageCharIndexMap.push_back(lineCharIndices);
    }
    
    // 添加剩余行
    if (!currentPage.empty()) {
        PageRenderData pageData;
        pageData.pageIndex = static_cast<int>(pageDataList.size());
        pageData.lines = currentPage;
        pageData.scaledWidth = scaledWidth;
        pageData.scaledHeight = scaledHeight;
        pageData.params = m_params;
        pageData.rng.seed(seedRng());
        pageData.charIndexMap = currentPageCharIndexMap;
        pageDataList.push_back(pageData);
    }
    
    // 确保至少有一页
    if (pageDataList.empty()) {
        PageRenderData pageData;
        pageData.pageIndex = 0;
        pageData.scaledWidth = scaledWidth;
        pageData.scaledHeight = scaledHeight;
        pageData.params = m_params;
        pageData.rng.seed(seedRng());
        pageDataList.push_back(pageData);
    }
    
    int totalPages = static_cast<int>(pageDataList.size());
    
    // 总步骤数：渲染 + 保存 = 2 * 总页数
    int totalSteps = totalPages * 2;
    
    if (progressCallback) {
        progressCallback(0, totalSteps);
    }
    
    // 并行渲染页面
    QThreadPool::globalInstance()->setMaxThreadCount(threadCount);
    
    // 先渲染所有页面
    QFuture<QImage> future = QtConcurrent::mapped(pageDataList, renderPageStatic);
    
    int renderCompleted = 0;
    while (!future.isFinished()) {
        QThread::msleep(50);
        // 检查渲染进度
        int completed = 0;
        for (int i = 0; i < totalPages; ++i) {
            if (future.isResultReadyAt(i)) {
                completed++;
            }
        }
        if (completed != renderCompleted && progressCallback) {
            progressCallback(completed, totalSteps);
            renderCompleted = completed;
        }
    }
    
    // 获取所有结果
    QList<QImage> images = future.results();
    
    // 保存图像并跟踪进度
    for (int i = 0; i < images.size() && i < totalPages; ++i) {
        QString filePath = QString::fromStdString(outputDir) + "/" + QString::number(i) + ".png";
        if (images[i].save(filePath, "PNG")) {
            filePaths[i] = filePath.toStdString();
        }
        // 更新保存步骤的进度
        if (progressCallback) {
            progressCallback(totalPages + i + 1, totalSteps);
        }
    }
    
    return filePaths;
}

std::vector<QChar> HandwriteGenerator::findUnsupportedChars(const std::string& text) {
    return findUnsupportedCharsStatic(text, m_params.fontPath);
}

std::vector<QChar> HandwriteGenerator::findUnsupportedCharsStatic(const std::string& text,
                                                                    const std::string& fontPath) {
    std::vector<QChar> unsupportedChars;
    std::set<QChar> seenChars;  // 用于去重
    
    // 加载字体
    QFont font;
    if (!fontPath.empty()) {
        QString fontPathQ = QString::fromStdString(fontPath);
        int fontId = QFontDatabase::addApplicationFont(fontPathQ);
        if (fontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                font = QFont(families[0]);
            }
        }
    }
    
    QFontMetrics fm(font);
    QString qText = QString::fromStdString(text);
    
    for (const QChar& c : qText) {
        // 跳过空白和控制字符
        if (c.isSpace() || c.category() == QChar::Other_Control) {
            continue;
        }
        
        // 检查字体是否支持该字符
        if (!fm.inFont(c) && seenChars.find(c) == seenChars.end()) {
            unsupportedChars.push_back(c);
            seenChars.insert(c);
        }
    }
    
    return unsupportedChars;
}

} // namespace HandWrite