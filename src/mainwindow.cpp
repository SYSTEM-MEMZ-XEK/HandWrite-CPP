#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QScrollBar>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QThread>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QMimeData>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QPrinter>
#include <QPrintDialog>

namespace HandWrite {

//=============================================================================
// 字符覆盖对话框实现
//=============================================================================

CharacterOverrideDialog::CharacterOverrideDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置字符属性"));
    setModal(false);  // 非模态对话框，允许查看预览变化
    
    auto *mainLayout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();
    
    // Font size override
    auto *fontSizeLayout = new QHBoxLayout();
    m_fontSizeCheck = new QCheckBox(tr("覆盖"), this);
    m_fontSizeSpin = new QSpinBox(this);
    m_fontSizeSpin->setRange(1, 200);
    m_fontSizeSpin->setValue(30);
    m_fontSizeSpin->setEnabled(false);
    fontSizeLayout->addWidget(m_fontSizeCheck);
    fontSizeLayout->addWidget(m_fontSizeSpin);
    fontSizeLayout->addStretch();
    formLayout->addRow(tr("字体大小:"), fontSizeLayout);
    connect(m_fontSizeCheck, &QCheckBox::toggled, m_fontSizeSpin, &QSpinBox::setEnabled);
    
    // Horizontal offset override
    auto *perturbXLayout = new QHBoxLayout();
    m_perturbXCheck = new QCheckBox(tr("覆盖"), this);
    m_perturbXSpin = new QDoubleSpinBox(this);
    m_perturbXSpin->setRange(-100, 100);
    m_perturbXSpin->setValue(0);
    m_perturbXSpin->setDecimals(1);
    m_perturbXSpin->setEnabled(false);
    perturbXLayout->addWidget(m_perturbXCheck);
    perturbXLayout->addWidget(m_perturbXSpin);
    perturbXLayout->addStretch();
    formLayout->addRow(tr("横向偏移:"), perturbXLayout);
    connect(m_perturbXCheck, &QCheckBox::toggled, m_perturbXSpin, &QDoubleSpinBox::setEnabled);
    
    // Vertical offset override
    auto *perturbYLayout = new QHBoxLayout();
    m_perturbYCheck = new QCheckBox(tr("覆盖"), this);
    m_perturbYSpin = new QDoubleSpinBox(this);
    m_perturbYSpin->setRange(-100, 100);
    m_perturbYSpin->setValue(0);
    m_perturbYSpin->setDecimals(1);
    m_perturbYSpin->setEnabled(false);
    perturbYLayout->addWidget(m_perturbYCheck);
    perturbYLayout->addWidget(m_perturbYSpin);
    perturbYLayout->addStretch();
    formLayout->addRow(tr("纵向偏移:"), perturbYLayout);
    connect(m_perturbYCheck, &QCheckBox::toggled, m_perturbYSpin, &QDoubleSpinBox::setEnabled);
    
    // Rotation override
    auto *perturbThetaLayout = new QHBoxLayout();
    m_perturbThetaCheck = new QCheckBox(tr("覆盖"), this);
    m_perturbThetaSpin = new QDoubleSpinBox(this);
    m_perturbThetaSpin->setRange(-180, 180);
    m_perturbThetaSpin->setValue(0);
    m_perturbThetaSpin->setDecimals(2);
    m_perturbThetaSpin->setSuffix(tr("°"));
    m_perturbThetaSpin->setEnabled(false);
    perturbThetaLayout->addWidget(m_perturbThetaCheck);
    perturbThetaLayout->addWidget(m_perturbThetaSpin);
    perturbThetaLayout->addStretch();
    formLayout->addRow(tr("旋转角度:"), perturbThetaLayout);
    connect(m_perturbThetaCheck, &QCheckBox::toggled, m_perturbThetaSpin, &QDoubleSpinBox::setEnabled);
    
    // 颜色覆盖
    auto *colorLayout = new QHBoxLayout();
    m_colorCheck = new QCheckBox(tr("覆盖"), this);
    m_colorRedSpin = new QSpinBox(this);
    m_colorRedSpin->setRange(0, 255);
    m_colorRedSpin->setValue(0);
    m_colorRedSpin->setEnabled(false);
    m_colorGreenSpin = new QSpinBox(this);
    m_colorGreenSpin->setRange(0, 255);
    m_colorGreenSpin->setValue(0);
    m_colorGreenSpin->setEnabled(false);
    m_colorBlueSpin = new QSpinBox(this);
    m_colorBlueSpin->setRange(0, 255);
    m_colorBlueSpin->setValue(0);
    m_colorBlueSpin->setEnabled(false);
    m_colorAlphaSpin = new QSpinBox(this);
    m_colorAlphaSpin->setRange(0, 255);
    m_colorAlphaSpin->setValue(255);
    m_colorAlphaSpin->setEnabled(false);
    colorLayout->addWidget(m_colorCheck);
    colorLayout->addWidget(new QLabel("R:", this));
    colorLayout->addWidget(m_colorRedSpin);
    colorLayout->addWidget(new QLabel("G:", this));
    colorLayout->addWidget(m_colorGreenSpin);
    colorLayout->addWidget(new QLabel("B:", this));
    colorLayout->addWidget(m_colorBlueSpin);
    colorLayout->addWidget(new QLabel("A:", this));
    colorLayout->addWidget(m_colorAlphaSpin);
    formLayout->addRow(tr("字体颜色:"), colorLayout);
    connect(m_colorCheck, &QCheckBox::toggled, m_colorRedSpin, &QSpinBox::setEnabled);
    connect(m_colorCheck, &QCheckBox::toggled, m_colorGreenSpin, &QSpinBox::setEnabled);
    connect(m_colorCheck, &QCheckBox::toggled, m_colorBlueSpin, &QSpinBox::setEnabled);
    connect(m_colorCheck, &QCheckBox::toggled, m_colorAlphaSpin, &QSpinBox::setEnabled);
    
    mainLayout->addLayout(formLayout);
    
    // Dialog buttons with Apply
    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::clicked, this, [this, buttonBox](QAbstractButton *button) {
        auto role = buttonBox->buttonRole(button);
        if (role == QDialogButtonBox::ApplyRole) {
            onApplyClicked();
        } else if (role == QDialogButtonBox::AcceptRole) {
            onApplyClicked();
            accept();
        } else if (role == QDialogButtonBox::RejectRole) {
            reject();
        }
    });
    mainLayout->addWidget(buttonBox);
}

void CharacterOverrideDialog::onApplyClicked() {
    emit applied();
}

void CharacterOverrideDialog::setOverride(const CharacterOverride &override) {
    if (override.fontSize.has_value()) {
        m_fontSizeCheck->setChecked(true);
        m_fontSizeSpin->setValue(*override.fontSize);
    }
    if (override.perturbX.has_value()) {
        m_perturbXCheck->setChecked(true);
        m_perturbXSpin->setValue(*override.perturbX);
    }
    if (override.perturbY.has_value()) {
        m_perturbYCheck->setChecked(true);
        m_perturbYSpin->setValue(*override.perturbY);
    }
    if (override.perturbTheta.has_value()) {
        m_perturbThetaCheck->setChecked(true);
        // Convert radians to degrees for display
        m_perturbThetaSpin->setValue(*override.perturbTheta * 180.0 / M_PI);
    }
    if (override.fillColor.has_value()) {
        m_colorCheck->setChecked(true);
        m_colorRedSpin->setValue(override.fillColor->r);
        m_colorGreenSpin->setValue(override.fillColor->g);
        m_colorBlueSpin->setValue(override.fillColor->b);
        m_colorAlphaSpin->setValue(override.fillColor->a);
    }
}

CharacterOverride CharacterOverrideDialog::getOverride() const {
    CharacterOverride override;
    
    if (m_fontSizeCheck->isChecked()) {
        override.fontSize = m_fontSizeSpin->value();
    }
    if (m_perturbXCheck->isChecked()) {
        override.perturbX = m_perturbXSpin->value();
    }
    if (m_perturbYCheck->isChecked()) {
        override.perturbY = m_perturbYSpin->value();
    }
    if (m_perturbThetaCheck->isChecked()) {
        // Convert degrees to radians
        override.perturbTheta = m_perturbThetaSpin->value() * M_PI / 180.0;
    }
    if (m_colorCheck->isChecked()) {
        override.fillColor = Color(
            static_cast<unsigned char>(m_colorRedSpin->value()),
            static_cast<unsigned char>(m_colorGreenSpin->value()),
            static_cast<unsigned char>(m_colorBlueSpin->value()),
            static_cast<unsigned char>(m_colorAlphaSpin->value())
        );
    }
    
    return override;
}

//=============================================================================
// MainWindow Implementation
//=============================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_scene(new QGraphicsScene(this))
    , m_pixmapItem(nullptr)
    , m_previewWatcher(new QFutureWatcher<std::vector<QImage>>(this))
    , m_exportWatcher(new QFutureWatcher<std::map<int, std::string>>(this))
    , m_progressDialog(nullptr)
{
    ui->setupUi(this);
    
    // Set up graphics view
    ui->imgPreview->setScene(m_scene);
    ui->imgPreview->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->imgPreview->setRenderHint(QPainter::Antialiasing);
    ui->imgPreview->setRenderHint(QPainter::SmoothPixmapTransform);
    ui->imgPreview->installEventFilter(this);
    
    // 为文本编辑框安装纯文本粘贴事件过滤器
    ui->textEditMain->installEventFilter(this);
    
    setupConnections();
    setupDefaults();
    populateComboBoxes();
    
    // Connect async watchers
    connect(m_previewWatcher, &QFutureWatcher<std::vector<QImage>>::finished, 
            this, &MainWindow::onPreviewFinished);
    connect(m_exportWatcher, &QFutureWatcher<std::map<int, std::string>>::finished,
            this, &MainWindow::onExportFinished);
    
    // Initial preview
    updatePreview();
}

MainWindow::~MainWindow() {
    // 取消并等待运行中的任务
    if (m_previewWatcher) {
        if (m_previewWatcher->isRunning()) {
            m_previewWatcher->cancel();
            m_previewWatcher->waitForFinished();
        }
        delete m_previewWatcher;
        m_previewWatcher = nullptr;
    }
    
    if (m_exportWatcher) {
        if (m_exportWatcher->isRunning()) {
            m_exportWatcher->cancel();
            m_exportWatcher->waitForFinished();
        }
        delete m_exportWatcher;
        m_exportWatcher = nullptr;
    }
    
    // Clean up progress dialog
    if (m_progressDialog) {
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    // Clear preview images to free memory
    m_previewImages.clear();
    m_previewImages.shrink_to_fit();
    m_previewImagePaths.clear();
    
    // Clear scene
    if (m_scene) {
        m_scene->clear();
    }
    
    delete ui;
}

void MainWindow::setupConnections() {
    connect(ui->pushButtonPreview, &QPushButton::clicked, this, &MainWindow::onPushButtonPreviewClicked);
    connect(ui->pushButtonExport, &QPushButton::clicked, this, &MainWindow::onPushButtonExportClicked);
    connect(ui->pushButtonPrint, &QPushButton::clicked, this, &MainWindow::onPushButtonPrintClicked);
    connect(ui->pushButtonSaveConfig, &QPushButton::clicked, this, &MainWindow::onPushButtonSaveConfigClicked);
    connect(ui->pushButtonLoadConfig, &QPushButton::clicked, this, &MainWindow::onPushButtonLoadConfigClicked);
    connect(ui->pushButtonCharOverride, &QPushButton::clicked, this, &MainWindow::onPushButtonCharOverrideClicked);
    connect(ui->pushButtonClearOverrides, &QPushButton::clicked, this, &MainWindow::onPushButtonClearOverridesClicked);
    connect(ui->comboBoxPaperTemplate, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onComboBoxPaperTemplateCurrentIndexChanged);
    
    // Pagination connections
    connect(ui->pushButtonFirstPage, &QPushButton::clicked, this, &MainWindow::onPushButtonFirstPageClicked);
    connect(ui->pushButtonPrevPage, &QPushButton::clicked, this, &MainWindow::onPushButtonPrevPageClicked);
    connect(ui->pushButtonNextPage, &QPushButton::clicked, this, &MainWindow::onPushButtonNextPageClicked);
    connect(ui->pushButtonLastPage, &QPushButton::clicked, this, &MainWindow::onPushButtonLastPageClicked);
    connect(ui->spinBoxPage, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onSpinBoxPageValueChanged);
    
    // Zoom connections
    connect(ui->pushButtonZoomIn, &QPushButton::clicked, this, &MainWindow::onPushButtonZoomInClicked);
    connect(ui->pushButtonZoomOut, &QPushButton::clicked, this, &MainWindow::onPushButtonZoomOutClicked);
    connect(ui->pushButtonZoomReset, &QPushButton::clicked, this, &MainWindow::onPushButtonZoomResetClicked);
    connect(ui->sliderZoom, &QSlider::valueChanged, this, &MainWindow::onSliderZoomValueChanged);
}

void MainWindow::setupDefaults() {
    // Initialize zoom
    m_zoomFactor = 1.0;
    updateZoomDisplay();
    
    // Default text
    ui->textEditMain->setPlainText(
        "使用 C++ 编写的手写字生成器，旨在完成一些无用的手写作业任务"
        "本项目提供了丰富的参数设置，以满足您在生成手写字时的个性化需求"
    );
    
    // Initialize pagination state
    m_currentPage = 0;
    m_totalPages = 0;
    updatePaginationUI();
}

void MainWindow::populateComboBoxes() {
    // Set TTF library path to executable directory
    QString exeDir = QCoreApplication::applicationDirPath();
    QString ttfPath = exeDir + "/ttf_library";
    m_tools.setTtfLibraryPath(ttfPath.toStdString());
    
    // Populate font selector
    auto [fontNames, fontPaths] = m_tools.getTtfFiles();
    ui->comboBoxFont->clear();
    for (const auto& name : fontNames) {
        ui->comboBoxFont->addItem(QString::fromStdString(name));
    }
    if (!fontPaths.empty()) {
        m_generator.setFont(fontPaths[0], m_generator.templateParams().fontSize);
    }
    
    // Populate character color selector
    ui->comboBoxCharColor->clear();
    for (const auto& [name, color] : m_tools.fontColors()) {
        ui->comboBoxCharColor->addItem(QString::fromStdString(name));
    }
    
    // Populate background color selector
    ui->comboBoxBackgroundColor->clear();
    for (const auto& [name, color] : m_tools.backgroundColors()) {
        ui->comboBoxBackgroundColor->addItem(QString::fromStdString(name));
    }
    
    // Populate resolution selector
    ui->comboBoxResolution->clear();
    for (const auto& [name, rate] : m_tools.resolutionRates()) {
        ui->comboBoxResolution->addItem(QString::fromStdString(name));
    }
    ui->comboBoxResolution->setCurrentIndex(2); // Default to x4
    
    // Populate paper template selector
    ui->comboBoxPaperTemplate->clear();
    for (const auto& [key, size] : m_tools.paperSizes()) {
        ui->comboBoxPaperTemplate->addItem(QString::fromStdString(size.name));
    }
    ui->comboBoxPaperTemplate->setCurrentIndex(0); // Default to "默认"
}

void MainWindow::showImage(const QString &imagePath) {
    QImage image(imagePath);
    if (image.isNull()) {
        qDebug() << "Failed to load image:" << imagePath;
        return;
    }
    showImage(image);
}

void MainWindow::showImage(const QImage &image) {
    // 存储原始像素图用于缩放
    QPixmap pixmap = QPixmap::fromImage(image);
    
    // 清除场景并创建新的像素图项
    m_scene->clear();
    m_pixmapItem = m_scene->addPixmap(pixmap);
    m_scene->setSceneRect(pixmap.rect());
    
    // 应用当前缩放
    applyZoom();
    
    // 居中视图
    ui->imgPreview->centerOn(m_pixmapItem);
}

TemplateParams MainWindow::getParamsFromForm() {
    TemplateParams params = m_generator.templateParams();
    
    // Paper size
    params.paperWidth = ui->lineEditWidth->text().toInt();
    params.paperHeight = ui->lineEditHeight->text().toInt();
    
    // Font
    auto fontPaths = m_tools.getTtfFilePaths();
    int fontIndex = ui->comboBoxFont->currentIndex();
    if (fontIndex >= 0 && fontIndex < static_cast<int>(fontPaths.size())) {
        params.fontPath = fontPaths[fontIndex];
    }
    params.fontSize = ui->lineEditFontSize->text().toInt();
    
    // Spacing
    params.lineSpacing = ui->lineEditLineSpacing->text().toInt();
    params.wordSpacing = ui->lineEditCharDistance->text().toInt();
    
    // Margins
    params.topMargin = ui->lineEditMarginTop->text().toInt();
    params.bottomMargin = ui->lineEditMarginBottom->text().toInt();
    params.leftMargin = ui->lineEditMarginLeft->text().toInt();
    params.rightMargin = ui->lineEditMarginRight->text().toInt();
    
    // Colors
    params.fillColor = m_tools.getFontColor(ui->comboBoxCharColor->currentText().toStdString());
    params.backgroundColor = m_tools.getBackgroundColor(ui->comboBoxBackgroundColor->currentText().toStdString());
    
    // Resolution
    params.rate = m_tools.getResolutionRate(ui->comboBoxResolution->currentText().toStdString());
    
    // Perturbations
    params.lineSpacingSigma = ui->lineEditLineSpacingSigma->text().toDouble();
    params.fontSizeSigma = ui->lineEditFontSizeSigma->text().toDouble();
    params.wordSpacingSigma = ui->lineEditWordSpacingSigma->text().toDouble();
    params.perturbXSigma = ui->lineEditPerturbXSigma->text().toDouble();
    params.perturbYSigma = ui->lineEditPerturbYSigma->text().toDouble();
    params.perturbThetaSigma = ui->lineEditPerturbThetaSigma->text().toDouble();
    
    // Character overrides
    params.charOverrides = m_charOverrides;
    
    return params;
}

QString MainWindow::getTextFromTextEdit() {
    return ui->textEditMain->toPlainText();
}

void MainWindow::updatePreview() {
    // Skip if async operation is running
    if (m_previewWatcher->isRunning() || m_exportWatcher->isRunning()) {
        return;
    }
    
    // Get parameters
    TemplateParams params = getParamsFromForm();
    QString text = getTextFromTextEdit();
    
    // Store the user's selected rate for export
    m_exportRate = params.rate;
    
    // Use lower rate for preview (max 4 for performance)
    int previewRate = qMin(params.rate, 4);
    
    // Start async generation (without progress dialog for initial preview)
    QFuture<std::vector<QImage>> future = QtConcurrent::run([this, params, text, previewRate]() {
        return generatePreviewAsync(params, text, previewRate);
    });
    m_previewWatcher->setFuture(future);
}

void MainWindow::onPushButtonPreviewClicked() {
    // Check if already running
    if (m_previewWatcher->isRunning() || m_exportWatcher->isRunning()) {
        QMessageBox::warning(this, tr("请稍候"), tr("正在处理中，请稍候..."));
        return;
    }
    
    // Get parameters
    TemplateParams params = getParamsFromForm();
    QString text = getTextFromTextEdit();
    
    // Store the user's selected rate for export
    m_exportRate = params.rate;
    
    // Use lower rate for preview (max 4 for performance)
    int previewRate = qMin(params.rate, 4);
    
    setupProgressDialog(tr("正在生成预览..."));
    
    // Start async generation
    QFuture<std::vector<QImage>> future = QtConcurrent::run([this, params, text, previewRate]() {
        return generatePreviewAsync(params, text, previewRate);
    });
    m_previewWatcher->setFuture(future);
}

std::vector<QImage> MainWindow::generatePreviewAsync(TemplateParams params, QString text, int previewRate) {
    HandwriteGenerator generator;
    params.rate = previewRate;
    generator.modifyTemplateParams(params);
    return generator.generatePreviewParallel(text.toStdString());
}

void MainWindow::onPreviewFinished() {
    if (m_progressDialog) {
        m_progressDialog->close();
    }
    
    if (m_previewWatcher->isCanceled()) {
        return;
    }
    
    m_previewImages = m_previewWatcher->result();
    
    // Update pagination state
    m_totalPages = static_cast<int>(m_previewImages.size());
    m_currentPage = 0;
    
    // Update pagination UI
    updatePaginationUI();
    
    // Show first image
    if (!m_previewImages.empty()) {
        showImage(m_previewImages[0]);
    }
    
    // 检测字体不支持的字符（生僻字）
    TemplateParams params = getParamsFromForm();
    QString text = getTextFromTextEdit();
    std::vector<QChar> unsupportedChars = HandwriteGenerator::findUnsupportedCharsStatic(
        text.toStdString(), params.fontPath);
    
    // Show status message
    statusBar()->showMessage(tr("预览已生成，共 %1 页").arg(m_previewImages.size()), 3000);
    
    // 如果有生僻字，显示提示
    if (!unsupportedChars.empty()) {
        QString charList;
        for (size_t i = 0; i < unsupportedChars.size() && i < 20; ++i) {
            charList += unsupportedChars[i];
            if (i < unsupportedChars.size() - 1 && i < 19) {
                charList += " ";
            }
        }
        if (unsupportedChars.size() > 20) {
            charList += tr(" ...等共 %1 个").arg(unsupportedChars.size());
        }
        
        QMessageBox::warning(this, tr("生僻字提示"), 
            tr("以下 %1 个字符在当前字体中不存在，将显示为空白：\n\n%2\n\n建议更换字体或手动补充这些字符。")
            .arg(unsupportedChars.size()).arg(charList));
    }
}

void MainWindow::onPushButtonExportClicked() {
    // Check if already running
    if (m_previewWatcher->isRunning() || m_exportWatcher->isRunning()) {
        QMessageBox::warning(this, tr("请稍候"), tr("正在处理中，请稍候..."));
        return;
    }
    
    // Clear output directory
    QString outputDir = "outputs";
    QDir dir(outputDir);
    if (dir.exists()) {
        QStringList files = dir.entryList(QDir::Files);
        for (const QString& file : files) {
            dir.remove(file);
        }
    } else {
        dir.mkpath(".");
    }
    
    // Get parameters
    TemplateParams params = getParamsFromForm();
    QString text = getTextFromTextEdit();
    
    // Warn for very high resolution
    if (params.rate >= 16) {
        auto reply = QMessageBox::question(this, tr("高分辨率警告"), 
            tr("高分辨率 (x%1) 生成可能需要较长时间和大量内存。\n\n确定要继续吗？").arg(params.rate),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    
    setupProgressDialog(tr("正在导出图片..."));
    
    // Start async export
    QFuture<std::map<int, std::string>> future = QtConcurrent::run([this, params, text, outputDir]() {
        return generateExportAsync(params, text, outputDir);
    });
    m_exportWatcher->setFuture(future);
}

void MainWindow::onPushButtonPrintClicked() {
    // 检查是否有预览内容
    if (m_previewImages.empty() && m_previewImagePaths.empty()) {
        QMessageBox::warning(this, tr("无法打印"), tr("请先预览或导出内容后再打印。"));
        return;
    }
    
    // 创建打印机对象
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    
    // 显示打印对话框
    QPrintDialog printDialog(&printer, this);
    printDialog.setWindowTitle(tr("打印预览"));
    
    if (printDialog.exec() != QDialog::Accepted) {
        return;
    }
    
    // 准备打印
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::warning(this, tr("打印失败"), tr("无法启动打印任务。"));
        return;
    }
    
    // 获取打印页面的尺寸
    QRectF pageRect = printer.pageLayout().paintRectPixels(printer.resolution());
    double pageWidth = pageRect.width();
    double pageHeight = pageRect.height();
    
    // 打印所有页面
    int totalPages = qMax(static_cast<int>(m_previewImages.size()), 
                          static_cast<int>(m_previewImagePaths.size()));
    
    for (int page = 0; page < totalPages; ++page) {
        if (page > 0) {
            printer.newPage();
        }
        
        QImage image;
        
        // 优先从预览图像获取
        if (page < static_cast<int>(m_previewImages.size())) {
            image = m_previewImages[page];
        } else {
            // 从导出的文件路径获取
            auto it = m_previewImagePaths.find(page);
            if (it != m_previewImagePaths.end()) {
                image.load(QString::fromStdString(it->second));
            }
        }
        
        if (image.isNull()) {
            continue;
        }
        
        // 计算缩放比例，保持宽高比
        double scaleX = pageWidth / image.width();
        double scaleY = pageHeight / image.height();
        double scale = qMin(scaleX, scaleY);
        
        // 计算居中位置
        int scaledWidth = static_cast<int>(image.width() * scale);
        int scaledHeight = static_cast<int>(image.height() * scale);
        int x = static_cast<int>((pageWidth - scaledWidth) / 2);
        int y = static_cast<int>((pageHeight - scaledHeight) / 2);
        
        // 绘制图像
        painter.drawImage(QRect(x, y, scaledWidth, scaledHeight), image);
    }
    
    painter.end();
    
    statusBar()->showMessage(tr("打印任务已发送"), 3000);
}

std::map<int, std::string> MainWindow::generateExportAsync(TemplateParams params, QString text, QString outputDir) {
    HandwriteGenerator generator;
    generator.modifyTemplateParams(params);
    
    // Use parallel generation with progress callback
    return generator.generateImageParallel(text.toStdString(), outputDir.toStdString(), 0,
        [this](int current, int total) {
            // Progress callback - update dialog on main thread
            QMetaObject::invokeMethod(this, [this, current, total]() {
                if (m_progressDialog) {
                    int totalPages = total / 2;
                    m_progressDialog->setMaximum(total);
                    m_progressDialog->setValue(current);
                    
                    QString statusText;
                    if (current <= totalPages) {
                        // Rendering phase
                        statusText = tr("正在渲染... %1/%2 页").arg(current).arg(totalPages);
                    } else {
                        // Saving phase
                        int savedPages = current - totalPages;
                        statusText = tr("正在保存... %1/%2 页").arg(savedPages).arg(totalPages);
                    }
                    m_progressDialog->setLabelText(statusText);
                }
            }, Qt::QueuedConnection);
        });
}

void MainWindow::onExportFinished() {
    if (m_exportWatcher->isCanceled()) {
        if (m_progressDialog) {
            m_progressDialog->close();
        }
        return;
    }
    
    m_previewImagePaths = m_exportWatcher->result();
    int totalPages = static_cast<int>(m_previewImagePaths.size());
    int totalSteps = totalPages * 2;
    
    // Show 100% progress and "全部导出" status
    if (m_progressDialog) {
        m_progressDialog->setMaximum(totalSteps > 0 ? totalSteps : 1);
        m_progressDialog->setValue(totalSteps > 0 ? totalSteps : 1);
        m_progressDialog->setLabelText(tr("全部导出"));
        // Force UI update
        QCoreApplication::processEvents();
        // Brief delay to show the completion state
        QThread::msleep(500);
        m_progressDialog->close();
    }
    
    // Update pagination state
    m_totalPages = totalPages;
    m_currentPage = 0;
    
    // Update pagination UI
    updatePaginationUI();
    
    // Show first image
    if (!m_previewImagePaths.empty()) {
        showImage(QString::fromStdString(m_previewImagePaths[0]));
    }
    
    // Show success message with "Open Directory" button
    QString outputDir = "outputs";
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("导出完成"));
    msgBox.setText(tr("已导出 %1 页到 %2 目录").arg(totalPages).arg(outputDir));
    msgBox.setIcon(QMessageBox::Information);
    QPushButton *openDirBtn = msgBox.addButton(tr("打开目录"), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
    
    if (msgBox.clickedButton() == openDirBtn) {
        // Open the output directory in file explorer
        QString absolutePath = QDir(outputDir).absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath));
    }
}

void MainWindow::setupProgressDialog(const QString &title, int maximum) {
    if (m_progressDialog) {
        delete m_progressDialog;
    }
    m_progressDialog = new QProgressDialog(title, tr("取消"), 0, maximum, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setCancelButton(nullptr); // 暂时禁用取消按钮
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->setValue(0);
    m_progressDialog->show();
}

void MainWindow::onPushButtonSaveConfigClicked() {
    QString path = QFileDialog::getSaveFileName(this, tr("保存配置文件"), 
                                                 "", tr("TOML Files (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    
    saveConfiguration(path);
    ui->labelCurrentConfig->setText(tr("当前配置文件:\n%1").arg(path));
}

void MainWindow::onPushButtonLoadConfigClicked() {
    QString path = QFileDialog::getOpenFileName(this, tr("加载配置文件"), 
                                                 "", tr("TOML Files (*.toml)"));
    if (path.isEmpty()) {
        return;
    }
    
    loadConfiguration(path);
    ui->labelCurrentConfig->setText(tr("当前配置文件:\n%1").arg(path));
}

void MainWindow::onPushButtonCharOverrideClicked() {
    QTextCursor cursor = ui->textEditMain->textCursor();
    int startPos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();
    
    if (startPos == endPos) {
        QMessageBox::warning(this, tr("提示"), tr("请先选中要设置属性的文本"));
        return;
    }
    
    // Ensure start < end
    if (startPos > endPos) {
        std::swap(startPos, endPos);
    }
    
    // Find existing override for this range
    CharacterOverride existingOverride;
    bool foundExisting = false;
    
    for (const auto &range : m_charOverrides) {
        if (range.startIndex == startPos && range.endIndex == endPos - 1) {
            existingOverride = range.override;
            foundExisting = true;
            break;
        }
    }
    
    // Show dialog (non-modal)
    CharacterOverrideDialog *dialog = new CharacterOverrideDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    if (foundExisting) {
        dialog->setOverride(existingOverride);
    }
    
    // Store range for use in signal handler
    struct OverrideRange {
        int start;
        int end;
    } range{startPos, endPos - 1};
    
    // Connect applied signal to update preview
    connect(dialog, &CharacterOverrideDialog::applied, this, [this, dialog, range]() {
        CharacterOverride override = dialog->getOverride();
        
        if (override.isEmpty()) {
            // Remove the override if it's empty
            m_charOverrides.erase(
                std::remove_if(m_charOverrides.begin(), m_charOverrides.end(),
                    [range](const CharacterOverrideRange &r) {
                        return r.startIndex == range.start && r.endIndex == range.end;
                    }),
                m_charOverrides.end());
        } else {
            // Remove existing override for this range (if any)
            m_charOverrides.erase(
                std::remove_if(m_charOverrides.begin(), m_charOverrides.end(),
                    [range](const CharacterOverrideRange &r) {
                        return r.startIndex == range.start && r.endIndex == range.end;
                    }),
                m_charOverrides.end());
            
            // Add new override
            CharacterOverrideRange newRange;
            newRange.startIndex = range.start;
            newRange.endIndex = range.end;
            newRange.override = override;
            m_charOverrides.push_back(newRange);
        }
        
        updateCharOverrideLabel();
        updatePreview();  // Refresh preview immediately
    });
    
    dialog->show();
}

void MainWindow::onPushButtonClearOverridesClicked() {
    if (m_charOverrides.empty()) {
        return;
    }
    
    m_charOverrides.clear();
    updateCharOverrideLabel();
    updatePreview();  // Refresh preview immediately
}

void MainWindow::updateCharOverrideLabel() {
    ui->labelCharOverride->setText(tr("字符覆盖: %1 处").arg(m_charOverrides.size()));
}

void MainWindow::saveConfiguration(const QString &path) {
    Config config;
    
    config.setWidth(ui->lineEditWidth->text().toInt());
    config.setHeight(ui->lineEditHeight->text().toInt());
    
    auto fontPaths = m_tools.getTtfFilePaths();
    int fontIndex = ui->comboBoxFont->currentIndex();
    if (fontIndex >= 0 && fontIndex < static_cast<int>(fontPaths.size())) {
        config.setTtfSelector(fontPaths[fontIndex]);
    }
    
    config.setFontSize(ui->lineEditFontSize->text().toInt());
    config.setLineSpacing(ui->lineEditLineSpacing->text().toInt());
    config.setCharDistance(ui->lineEditCharDistance->text().toInt());
    
    config.setMarginTop(ui->lineEditMarginTop->text().toInt());
    config.setMarginBottom(ui->lineEditMarginBottom->text().toInt());
    config.setMarginLeft(ui->lineEditMarginLeft->text().toInt());
    config.setMarginRight(ui->lineEditMarginRight->text().toInt());
    
    Color charColor = m_tools.getFontColor(ui->comboBoxCharColor->currentText().toStdString());
    config.setCharColor({charColor.r, charColor.g, charColor.b, charColor.a});
    
    Color bgColor = m_tools.getBackgroundColor(ui->comboBoxBackgroundColor->currentText().toStdString());
    config.setBackgroundColor({bgColor.r, bgColor.g, bgColor.b, bgColor.a});
    
    config.setResolution(m_tools.getResolutionRate(ui->comboBoxResolution->currentText().toStdString()));
    
    config.setLineSpacingSigma(ui->lineEditLineSpacingSigma->text().toDouble());
    config.setFontSizeSigma(ui->lineEditFontSizeSigma->text().toDouble());
    config.setWordSpacingSigma(ui->lineEditWordSpacingSigma->text().toDouble());
    config.setPerturbXSigma(ui->lineEditPerturbXSigma->text().toDouble());
    config.setPerturbYSigma(ui->lineEditPerturbYSigma->text().toDouble());
    config.setPerturbThetaSigma(ui->lineEditPerturbThetaSigma->text().toDouble());
    
    if (config.save(path.toStdString())) {
        QMessageBox::information(this, tr("保存成功"), tr("配置已保存到:\n%1").arg(path));
    } else {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存配置文件"));
    }
}

void MainWindow::loadConfiguration(const QString &path) {
    Config config(path.toStdString());
    
    // Width and Height
    if (auto val = config.width()) {
        ui->lineEditWidth->setText(QString::number(*val));
    }
    if (auto val = config.height()) {
        ui->lineEditHeight->setText(QString::number(*val));
    }
    
    // Font
    if (auto val = config.ttfSelector()) {
        auto fontPaths = m_tools.getTtfFilePaths();
        for (int i = 0; i < static_cast<int>(fontPaths.size()); ++i) {
            if (fontPaths[i] == *val) {
                ui->comboBoxFont->setCurrentIndex(i);
                break;
            }
        }
    }
    
    // Font size, line spacing, char distance
    if (auto val = config.fontSize()) {
        ui->lineEditFontSize->setText(QString::number(*val));
    }
    if (auto val = config.lineSpacing()) {
        ui->lineEditLineSpacing->setText(QString::number(*val));
    }
    if (auto val = config.charDistance()) {
        ui->lineEditCharDistance->setText(QString::number(*val));
    }
    
    // Margins
    if (auto val = config.marginTop()) {
        ui->lineEditMarginTop->setText(QString::number(*val));
    }
    if (auto val = config.marginBottom()) {
        ui->lineEditMarginBottom->setText(QString::number(*val));
    }
    if (auto val = config.marginLeft()) {
        ui->lineEditMarginLeft->setText(QString::number(*val));
    }
    if (auto val = config.marginRight()) {
        ui->lineEditMarginRight->setText(QString::number(*val));
    }
    
    // Colors
    if (auto val = config.charColor()) {
        for (const auto& [name, color] : m_tools.fontColors()) {
            if (color.r == (*val)[0] && color.g == (*val)[1] && 
                color.b == (*val)[2] && color.a == (*val)[3]) {
                ui->comboBoxCharColor->setCurrentText(QString::fromStdString(name));
                break;
            }
        }
    }
    
    if (auto val = config.backgroundColor()) {
        for (const auto& [name, color] : m_tools.backgroundColors()) {
            if (color.r == (*val)[0] && color.g == (*val)[1] && 
                color.b == (*val)[2] && color.a == (*val)[3]) {
                ui->comboBoxBackgroundColor->setCurrentText(QString::fromStdString(name));
                break;
            }
        }
    }
    
    // Resolution
    if (auto val = config.resolution()) {
        for (const auto& [name, rate] : m_tools.resolutionRates()) {
            if (rate == *val) {
                ui->comboBoxResolution->setCurrentText(QString::fromStdString(name));
                break;
            }
        }
    }
    
    // Perturbation sigmas
    if (auto val = config.lineSpacingSigma()) {
        ui->lineEditLineSpacingSigma->setText(QString::number(*val));
    }
    if (auto val = config.fontSizeSigma()) {
        ui->lineEditFontSizeSigma->setText(QString::number(*val));
    }
    if (auto val = config.wordSpacingSigma()) {
        ui->lineEditWordSpacingSigma->setText(QString::number(*val));
    }
    if (auto val = config.perturbXSigma()) {
        ui->lineEditPerturbXSigma->setText(QString::number(*val));
    }
    if (auto val = config.perturbYSigma()) {
        ui->lineEditPerturbYSigma->setText(QString::number(*val));
    }
    if (auto val = config.perturbThetaSigma()) {
        ui->lineEditPerturbThetaSigma->setText(QString::number(*val));
    }
    
    qDebug() << "Configuration loaded from" << path;
}

void MainWindow::onPushButtonFirstPageClicked() {
    goToPage(0);
}

void MainWindow::onPushButtonPrevPageClicked() {
    if (m_currentPage > 0) {
        goToPage(m_currentPage - 1);
    }
}

void MainWindow::onPushButtonNextPageClicked() {
    if (m_currentPage < m_totalPages - 1) {
        goToPage(m_currentPage + 1);
    }
}

void MainWindow::onPushButtonLastPageClicked() {
    if (m_totalPages > 0) {
        goToPage(m_totalPages - 1);
    }
}

void MainWindow::onSpinBoxPageValueChanged(int value) {
    // SpinBox uses 1-based indexing, internal uses 0-based
    goToPage(value - 1);
}

void MainWindow::goToPage(int page) {
    if (page < 0 || page >= m_totalPages || page == m_currentPage) {
        return;
    }
    
    m_currentPage = page;
    updatePaginationUI();
    
    // Try to show from preview images first, then from exported paths
    if (m_currentPage >= 0 && m_currentPage < static_cast<int>(m_previewImages.size())) {
        showImage(m_previewImages[m_currentPage]);
    } else if (!m_previewImagePaths.empty()) {
        // Find the image for current page in the map
        auto it = m_previewImagePaths.find(m_currentPage);
        if (it != m_previewImagePaths.end()) {
            showImage(QString::fromStdString(it->second));
        }
    }
}

void MainWindow::updatePaginationUI() {
    // 更新页面信息标签（显示使用1-based）
    ui->labelPageInfo->setText(tr("%1 / %2").arg(m_currentPage + 1).arg(m_totalPages));
    
    // 更新spinbox（阻止信号以防止递归调用）
    ui->spinBoxPage->blockSignals(true);
    ui->spinBoxPage->setRange(1, qMax(1, m_totalPages));
    ui->spinBoxPage->setValue(m_currentPage + 1);
    ui->spinBoxPage->blockSignals(false);
    
    // 更新按钮状态
    updatePageButtons();
}

void MainWindow::updatePageButtons() {
    bool hasPages = m_totalPages > 0;
    bool hasMultiplePages = m_totalPages > 1;
    
    ui->pushButtonFirstPage->setEnabled(hasMultiplePages && m_currentPage > 0);
    ui->pushButtonPrevPage->setEnabled(hasPages && m_currentPage > 0);
    ui->pushButtonNextPage->setEnabled(hasPages && m_currentPage < m_totalPages - 1);
    ui->pushButtonLastPage->setEnabled(hasMultiplePages && m_currentPage < m_totalPages - 1);
    ui->spinBoxPage->setEnabled(hasPages);
}

void MainWindow::onComboBoxPaperTemplateCurrentIndexChanged(int index) {
    if (index < 0) {
        return;
    }
    
    // Get the paper size keys in order
    std::vector<std::string> keys;
    for (const auto& [key, size] : m_tools.paperSizes()) {
        keys.push_back(key);
    }
    
    if (index >= static_cast<int>(keys.size())) {
        return;
    }
    
    const std::string& key = keys[index];
    PaperSize paperSize = m_tools.getPaperSize(key);
    
    // Update width and height fields
    ui->lineEditWidth->setText(QString::number(paperSize.width));
    ui->lineEditHeight->setText(QString::number(paperSize.height));
    
    // Enable/disable width/height fields based on whether it's custom
    bool isCustom = (key == "custom");
    ui->lineEditWidth->setReadOnly(!isCustom);
    ui->lineEditHeight->setReadOnly(!isCustom);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_PageUp:
            onPushButtonPrevPageClicked();
            break;
        case Qt::Key_Right:
        case Qt::Key_PageDown:
            onPushButtonNextPageClicked();
            break;
        case Qt::Key_Home:
            onPushButtonFirstPageClicked();
            break;
        case Qt::Key_End:
            onPushButtonLastPageClicked();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Check if there are running background tasks
    bool hasRunningTasks = (m_previewWatcher && m_previewWatcher->isRunning()) ||
                           (m_exportWatcher && m_exportWatcher->isRunning());
    
    if (hasRunningTasks) {
        // Ask user to confirm
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("确认关闭"),
            tr("正在处理中，关闭将取消当前操作。\n确定要关闭吗？"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
        
        // Cancel running tasks
        if (m_previewWatcher && m_previewWatcher->isRunning()) {
            m_previewWatcher->cancel();
            // Wait with timeout (max 2 seconds)
            m_previewWatcher->waitForFinished();
        }
        
        if (m_exportWatcher && m_exportWatcher->isRunning()) {
            m_exportWatcher->cancel();
            // Wait with timeout (max 2 seconds)
            m_exportWatcher->waitForFinished();
        }
    }
    
    // 清理进度对话框
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }
    
    // 清除预览图像以释放内存
    m_previewImages.clear();
    m_previewImages.shrink_to_fit();
    m_previewImagePaths.clear();
    
    // 清除场景
    if (m_scene) {
        m_scene->clear();
    }
    
    event->accept();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // Handle zoom for graphics view
    if (obj == ui->imgPreview && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        
        // Ctrl + Wheel for zoom
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            QPoint delta = wheelEvent->angleDelta();
            if (delta.y() > 0) {
                m_zoomFactor = qMin(m_zoomFactor + ZOOM_STEP, ZOOM_MAX);
            } else {
                m_zoomFactor = qMax(m_zoomFactor - ZOOM_STEP, ZOOM_MIN);
            }
            applyZoom();
            updateZoomDisplay();
            return true;
        }
    }
    
    // Handle paste for text edit - paste as plain text only
    if (obj == ui->textEditMain && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        // Check for Ctrl+V or Shift+Insert (paste shortcuts)
        if ((keyEvent->key() == Qt::Key_V && keyEvent->modifiers() & Qt::ControlModifier) ||
            (keyEvent->key() == Qt::Key_Insert && keyEvent->modifiers() & Qt::ShiftModifier)) {
            
            QClipboard *clipboard = QGuiApplication::clipboard();
            const QMimeData *mimeData = clipboard->mimeData();
            
            if (mimeData->hasText()) {
                // Insert plain text only, preserving newlines and spaces
                ui->textEditMain->insertPlainText(mimeData->text());
                return true;  // Event handled
            }
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onPushButtonZoomInClicked() {
    m_zoomFactor = qMin(m_zoomFactor + ZOOM_STEP, ZOOM_MAX);
    applyZoom();
    updateZoomDisplay();
}

void MainWindow::onPushButtonZoomOutClicked() {
    m_zoomFactor = qMax(m_zoomFactor - ZOOM_STEP, ZOOM_MIN);
    applyZoom();
    updateZoomDisplay();
}

void MainWindow::onPushButtonZoomResetClicked() {
    m_zoomFactor = 1.0;
    applyZoom();
    updateZoomDisplay();
}

void MainWindow::onSliderZoomValueChanged(int value) {
    // Slider value is 10-500, representing 10%-500%
    m_zoomFactor = value / 100.0;
    applyZoom();
}

void MainWindow::updateZoomDisplay() {
    // Update label
    ui->labelZoom->setText(QString("%1%").arg(static_cast<int>(m_zoomFactor * 100)));
    
    // Update slider (block signals to prevent recursion)
    ui->sliderZoom->blockSignals(true);
    ui->sliderZoom->setValue(static_cast<int>(m_zoomFactor * 100));
    ui->sliderZoom->blockSignals(false);
}

void MainWindow::applyZoom() {
    if (m_pixmapItem) {
        // 重置变换并应用新的缩放
        ui->imgPreview->resetTransform();
        ui->imgPreview->scale(m_zoomFactor, m_zoomFactor);
    }
}

} // namespace HandWrite
